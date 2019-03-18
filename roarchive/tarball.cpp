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
#include <map>

#include "dbglog/dbglog.hpp"

#include "utility/tar.hpp"
#include "utility/path.hpp"
#include "utility/substream.hpp"
#include "utility/streams.hpp"

#include "detail.hpp"
#include "io.hpp"

namespace fs = boost::filesystem;

namespace roarchive {

namespace {

class TarIStream : public IStream {
public:
    typedef utility::io::SubStreamDevice::Filedes Filedes;

    TarIStream(const fs::path &path, const Filedes &fd
               , const IStream::FilterInit &filterInit)
        : IStream(filterInit, (fd.end - fd.start)), path_(path)
    {
        fis_.push(utility::io::SubStreamDevice(path, fd));
    }

    virtual fs::path path() const { return path_; }
    virtual fs::path index() const { return path_; }
    virtual void close() {}

private:
    const fs::path path_;
};

HintedPath
findPrefix(const fs::path &path, const FileHint &hint
           , const utility::tar::Reader::File::list &files)
{
    if (!hint) { return {}; }

    // match all files
    FileHint::Matcher matcher(hint);
    for (const auto &file : files) {
        if (matcher(file.path)) {
            return HintedPath(file.path.parent_path(), file.path.filename());
        }
    }

    if (!matcher) {
        LOGTHROW(err2, std::runtime_error)
            << "No \"" << hint << "\" found in the tarball archive at "
            << path << ".";
    }

    return HintedPath(matcher.match().parent_path()
                      , matcher.match().filename());
}

class TarIndex {
public:
    typedef utility::io::SubStreamDevice::Filedes Filedes;

    TarIndex(utility::tar::Reader &reader, const OpenOptions &openOptions)
        : path_(reader.path()), files_(reader.files(openOptions.fileLimit))
        , fd_(reader.filedes())
        , prefix_(findPrefix(path_, openOptions.hint, files_))
    {
        for (const auto &file : files_) {
            if (!utility::isPathPrefix(file.path, prefix_.path)) { continue; }

            const auto path(utility::cutPathPrefix(file.path, prefix_.path));
            index_.insert(map::value_type
                              (path.string()
                               , { fd_, file.start, file.end() }));
        }
    }

    const Filedes& file(const std::string &path) const {
        auto findex(index_.find(path));
        if (findex == index_.end()) {
            LOGTHROW(err2, NoSuchFile)
                << "File \"" << path << "\" not found in the archive at "
                << path_ << ".";
        }
        return findex->second;
    }

    bool exists(const std::string &path) const {
        return (index_.find(path) != index_.end());
    }

    Files list() const {
        std::vector<boost::filesystem::path> list;
        for (const auto &pair : index_) {
            list.push_back(pair.first);
        }
        return list;
    }

    boost::optional<fs::path> findFile(const std::string &filename) const {
        for (const auto &pair : index_) {
            const fs::path path(pair.first);
            if (path.filename() == filename) { return path; }
        }
        return boost::none;
    }

    void applyHint(const FileHint &hint) {
        if (!hint) { return; }
        // regenerate
        prefix_ = findPrefix(path_, hint, files_);
        index_.clear();

        for (const auto &file : files_) {
            if (!utility::isPathPrefix(file.path, prefix_.path)) { continue; }

            const auto path(utility::cutPathPrefix(file.path, prefix_.path));
            index_.insert(map::value_type
                              (path.string()
                               , { fd_, file.start, file.end() }));
        }
    }

    const boost::optional<fs::path>& usedHint() const {
        return prefix_.usedHint;
    }

private:
    const fs::path path_;
    utility::tar::Reader::File::list files_;
    int fd_;
    typedef std::map<std::string, Filedes> map;
    map index_;
    HintedPath prefix_;
};

class Tarball : public RoArchive::Detail {
public:
    Tarball(const boost::filesystem::path &path
            , const OpenOptions &openOptions)
        : Detail(path), reader_(path), index_(reader_, openOptions)
    {}

    /** Get (wrapped) input stream for given file.
     *  Throws when not found.
     */
    virtual IStream::pointer istream(const boost::filesystem::path &path
                                     , const IStream::FilterInit &filterInit)
        const
    {
        return std::make_shared<TarIStream>(path, index_.file(path.string())
                                            , filterInit);
    }

    virtual bool exists(const boost::filesystem::path &path) const {

        return index_.exists(path.string());
    }

    virtual Files list() const {
        return index_.list();
    }

    virtual boost::optional<fs::path> findFile(const std::string &filename)
        const
    {
        return index_.findFile(filename);
    }

    virtual void applyHint(const FileHint &hint) {
        index_.applyHint(hint);
    }

    virtual const boost::optional<boost::filesystem::path>& usedHint() {
        return index_.usedHint();
    }

private:
    utility::tar::Reader reader_;
    TarIndex index_;
};

} // namespace

RoArchive::dpointer
RoArchive::tarball(const boost::filesystem::path &path
                   , const OpenOptions &openOptions)
{
    return std::make_shared<Tarball>(path, openOptions);
}

} // namespace roarchive
