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

#include "dbglog/dbglog.hpp"

#include "utility/streams.hpp"

#include "./detail.hpp"

namespace fs = boost::filesystem;

namespace roarchive {

namespace {

class FileIStream : public IStream {
public:
    FileIStream(const fs::path &path, const IStream::FilterInit &filterInit)
        : IStream(filterInit), path_(path)
    {
        try {
            stream_.open(path.string());
        } catch (const std::ios_base::failure &e) {
            LOGTHROW(err2, std::runtime_error)
                << "Cannot open file file " << path << ": " << e.what() << ".";
        }
        fis_.push(stream_);
    }

    virtual fs::path path() const { return path_; }
    virtual void close() { stream_.close(); }

private:
    const fs::path path_;
    utility::ifstreambuf stream_;
};

fs::path applyHintToPath(const fs::path &path
                         , const boost::optional<std::string> &hint)
{
    if (!hint) { return path; }

    auto hintPath([&]() -> boost::optional<fs::path>
    {
        for (fs::recursive_directory_iterator i(path), e; i != e; ++i) {
            if (i->path().filename() == *hint) { return i->path(); }
        }
        return boost::none;
    }());

    if (!hintPath) {
        LOGTHROW(err2, std::runtime_error)
            << "No \"" << *hint << "\" found in the zip archive at "
            << path << ".";
    }

    // use hint
    return hintPath->parent_path();
}

class Directory : public RoArchive::Detail {
public:
    Directory(const fs::path &path, const boost::optional<std::string> &hint)
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

    virtual std::vector<fs::path> list() const {
        std::vector<fs::path> list;
        for (fs::recursive_directory_iterator i(path_), e; i != e; ++i) {
            list.push_back(i->path());
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

    virtual void applyHint(const std::string &hint) {
        path_ = applyHintToPath(originalPath_, hint);
    }

private:
    const fs::path originalPath_;
};

} // namespace

RoArchive::dpointer
RoArchive::directory(const fs::path &path
                     , const boost::optional<std::string> &hint)
{
    return std::make_shared<Directory>(path, hint);
}

} // namespace roarchive

