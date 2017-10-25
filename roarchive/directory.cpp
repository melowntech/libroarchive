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

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>

#include "dbglog/dbglog.hpp"

#include "utility/path.hpp"

#include "./detail.hpp"
#include "./io.hpp"

namespace fs = boost::filesystem;
namespace bio = boost::iostreams;

namespace roarchive {

namespace {

class FileIStream : public IStream {
public:
    FileIStream(const fs::path &path, const IStream::FilterInit &filterInit)
        : IStream(filterInit), path_(path)
    {
        try {
            auto source(bio::file_source(path.string()));
            if (!source.is_open()) {
                // TODO: really? distinguish
                // should we use open(2)?
                LOGTHROW(err2, NoSuchFile)
                    << "Cannot open file file " << path << ".";
            }

            fis_.push(std::move(source));
        } catch (const std::ios_base::failure &e) {
            LOGTHROW(err2, Error)
                << "Cannot open file file " << path << ": " << e.what() << ".";
        }
    }

    virtual fs::path path() const { return path_; }
    virtual void close() {}

private:
    const fs::path path_;
};

fs::path applyHintToPath(const fs::path &path, const FileHint &hint)
{
    if (!hint) { return path; }

    auto hintPath([&]() -> boost::optional<fs::path>
    {
        FileHint::Matcher matcher(hint);
        for (fs::recursive_directory_iterator i(path), e; i != e; ++i) {
            if (matcher(i->path())) {
                return i->path().parent_path();
            }
        }

        if (matcher) { return matcher.match().parent_path(); }

        return boost::none;
    }());

    if (!hintPath) {
        LOGTHROW(err2, std::runtime_error)
            << "No \"" << hint << "\" found in the zip archive at "
            << path << ".";
    }

    // use hint
    return *hintPath;
}

class Directory : public RoArchive::Detail {
public:
    Directory(const fs::path &path, const FileHint &hint)
        : Detail(applyHintToPath(path, hint), true)
        , originalPath_(path)
    {}

    /** Get (wrapped) input stream for given file.
     *  Throws when not found.
     */
    virtual IStream::pointer istream(const fs::path &path
                                     , const IStream::FilterInit &filterInit)
        const
    {
        if (path.is_absolute()) {
            return std::make_shared<FileIStream>(path, filterInit);
        }
        return std::make_shared<FileIStream>(path_ / path, filterInit);
    }

    virtual bool exists(const fs::path &path) const {
        if (path.is_absolute()) {
            return fs::exists(path);
        }
        return fs::exists(path_ / path);
    }

    virtual Files list() const {
        Files list;
        for (fs::recursive_directory_iterator i(path_), e; i != e; ++i) {
            list.push_back(utility::cutPathPrefix(i->path(), path_));
        }
        return list;
    }

    virtual boost::optional<fs::path> findFile(const std::string &filename)
        const
    {
        for (fs::recursive_directory_iterator i(path_), e; i != e; ++i) {
            if (i->path().filename() == filename) { return i->path(); }
        }
        return boost::none;
    }

    virtual void applyHint(const FileHint &hint) {
        path_ = applyHintToPath(originalPath_, hint);
    }

private:
    const fs::path originalPath_;
};

} // namespace

RoArchive::dpointer
RoArchive::directory(const fs::path &path, const OpenOptions &openOptions)
{
    // do not apply any limit
    return std::make_shared<Directory>(path, openOptions.hint);
}

} // namespace roarchive

