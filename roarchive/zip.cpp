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

#include "utility/streams.hpp"
#include "utility/path.hpp"
#include "utility/zip.hpp"

#include "detail.hpp"
#include "io.hpp"

namespace fs = boost::filesystem;
namespace bio = boost::iostreams;

namespace roarchive {

namespace {

class ZipIStream : public IStream {
public:
    ZipIStream(const utility::zip::Reader &reader, std::size_t zipIndex
               , const IStream::FilterInit &filterInit
               , const fs::path &index)
        : IStream(filterInit), pf_(reader.plug(zipIndex, fis_))
        , index_(index)
    {
        update(pf_.uncompressedSize, pf_.seekable);
    }

    virtual fs::path path() const { return pf_.path; }
    virtual fs::path index() const { return index_; }
    virtual void close() {}

private:
    utility::zip::PluggedFile pf_;
    const fs::path index_;
};

HintedPath
findPrefix(const fs::path &path, const FileHint &hint
           , const utility::zip::Reader::Record::list &files)
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
            << "No \"" << hint << "\" found in the zip archive at "
            << path << ".";
    }

    return HintedPath(matcher.match().parent_path()
                      , matcher.match().filename());
}

class Zip : public RoArchive::Detail {
public:
    Zip(const boost::filesystem::path &path, const OpenOptions &openOptions)
        : Detail(path), reader_(path, openOptions.fileLimit)
        , prefix_(findPrefix(path, openOptions.hint, reader_.files()))
    {
        for (const auto &file : reader_.files()) {
            if (!utility::isPathPrefix(file.path, prefix_.path)) { continue; }

            const auto path(utility::cutPathPrefix(file.path, prefix_.path));
            index_.insert(map::value_type(path.string(), file));
        }
    }

    /** Get (wrapped) input stream for given file.
     *  Throws when not found.
     */
    virtual IStream::pointer istream(const boost::filesystem::path &path
                                     , const IStream::FilterInit &filterInit)
        const
    {
        auto findex(index_.find(path.string()));
        if (findex == index_.end()) {
            LOGTHROW(err2, NoSuchFile)
                << "File " << path << " not found in the zip archive at "
                << path_ << ".";
        }

        return std::make_shared<ZipIStream>
            (reader_, findex->second.index, filterInit, path);
    }

    virtual bool exists(const boost::filesystem::path &path) const {
        return (index_.find(path.string()) != index_.end());
    }

    virtual Files list() const {
        Files list;
        for (const auto &pair : index_) {
            list.push_back(pair.first);
        }
        return list;
    }

    virtual boost::optional<fs::path> findFile(const std::string &filename)
        const
    {
        for (const auto &pair : index_) {
            if (pair.second.path.filename() == filename) {
                return fs::path(pair.first);
            }
        }
        return boost::none;
    }

    virtual void applyHint(const FileHint &hint) {
        if (!hint) { return; }

        // regenerate
        prefix_ = findPrefix(path_, hint, reader_.files());
        index_.clear();

        for (const auto &file : reader_.files()) {
            if (!utility::isPathPrefix(file.path, prefix_.path)) { continue; }

            const auto path(utility::cutPathPrefix(file.path, prefix_.path));
            index_.insert(map::value_type(path.string(), file));
        }
    }

    virtual const boost::optional<boost::filesystem::path>& usedHint() {
        return prefix_.usedHint;
    }

private:
    utility::zip::Reader reader_;
    HintedPath prefix_;

    typedef std::map<std::string, utility::zip::Reader::Record> map;
    map index_;
};

} // namespace

RoArchive::dpointer RoArchive::zip(const boost::filesystem::path &path
                                   , const OpenOptions &openOptions)
{
    return std::make_shared<Zip>(path, openOptions);
}

} // namespace roarchive
