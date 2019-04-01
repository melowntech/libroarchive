/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <queue>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/array.hpp>

#include "dbglog/dbglog.hpp"

#include "utility/cppversion.hpp"
#include "utility/uri.hpp"

#include "http/ondemandclient.hpp"
#include "http/error.hpp"

#include "detail.hpp"
#include "io.hpp"

namespace fs = boost::filesystem;
namespace bio = boost::iostreams;

namespace roarchive {

namespace {

http::OnDemandClient client(4);

class HttpIStream : public IStream {
public:
    HttpIStream(const fs::path &path, const IStream::FilterInit &filterInit
                , const fs::path &index)
        : IStream(filterInit), path_(path), index_(index)
    {
        // TODO: make more robust
        const auto &fetcher(client.fetcher());

        auto q(fetcher.perform
               (utility::ResourceFetcher::Query(path.string())));

        if (q.ec()) {
            if (q.check(make_error_code(utility::HttpCode::NotFound))) {
                LOGTHROW(err2, NoSuchFile)
                    << "File at URL <" << path << "> doesn't exist.";
            }

            LOGTHROW(err1, IOError)
                << "Failed to download tile data from <"
                << path << ">: Unexpected HTTP status code: <"
                << q.ec() << ">.";
        }

        try {
            body_ = std::move(q.moveOut());
            const auto &data(body_.data);
            auto source(bio::array_source
                        (data.data(), data.data() + data.size()));
            fis_.push(std::move(source));
        } catch (const http::Error &e) {
            LOGTHROW(err1, IOError)
                << "Failed to download tile data from <"
                << path << ">: Unexpected error code <"
                << e.what() << ">.";
        }
    }

    virtual fs::path path() const { return path_; }
    virtual fs::path index() const { return index_; }
    virtual void close() {}

private:
    const fs::path path_;
    const fs::path index_;
    utility::ResourceFetcher::Query::Body body_;
};

HintedPath applyHintToPath(const fs::path &path, const FileHint &hint)
{
    if (!hint) { return path; }

    if (path.filename() != ".") {
        // some filename
        auto ppath(path.parent_path());
        ppath += '/';
        return HintedPath(ppath, path.filename());
    }

    // TODO: path ends with slash -> get hints and check files under remote path
    // (URI)

    return path;
}

struct HttpBase {
    HttpBase(const fs::path &path, const FileHint &hint)
        : hintedPath_(applyHintToPath(path, hint))
    {}

    HintedPath hintedPath_;
};

class Http
    : private HttpBase
    , public RoArchive::Detail
{
public:
    Http(const fs::path &path, const FileHint &hint)
        : HttpBase(path, hint)
        , Detail(hintedPath_.path, false)
        , originalPath_(path)
        , base_(path_.string())
    {}

    /** Get (wrapped) input stream for given file.
     *  Throws when not found.
     */
    virtual IStream::pointer istream(const fs::path &path
                                     , const IStream::FilterInit &filterInit)
        const
    {
        utility::Uri uri(path.string());
        if (uri.absolute()) {
            return std::make_unique<HttpIStream>(path, filterInit, path);
        }
        return std::make_unique<HttpIStream>
            (str(base_.resolve(utility::Uri(uri))), filterInit, path);
    }

    virtual bool exists(const fs::path &path) const {
        (void) path;
        return true;
    }

    virtual Files list() const {
        LOGTHROW(err2, NotImplemented)
            << "HTTP list not implemented.";
        throw;
    }

    virtual boost::optional<fs::path> findFile(const std::string&)
        const
    {
        LOGTHROW(err2, NotImplemented)
            << "HTTP find not implemented.";;
        throw;
    }

    virtual void applyHint(const FileHint &hint) {
        hintedPath_ = applyHintToPath(originalPath_, hint);
        path_ = hintedPath_.path;
        base_ = utility::Uri(path_.string());
    }

    virtual const boost::optional<boost::filesystem::path>& usedHint() {
        return hintedPath_.usedHint;
    }

    virtual bool handlesSchema(const std::string &schema) const {
        return ((schema == "http") || (schema == "https"));
    }

private:
    const fs::path originalPath_;
    utility::Uri base_;
};

} // namespace

RoArchive::dpointer
RoArchive::http(const fs::path &path, const OpenOptions &openOptions)
{
    // do not apply any limit
    return std::make_shared<Http>(path, openOptions.hint);
}

} // namespace roarchive
