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

#include "./detail.hpp"

namespace fs = boost::filesystem;
namespace bio = boost::iostreams;

namespace roarchive {

namespace {

class ZipIStream : public IStream {
public:
    ZipIStream(const utility::zip::Reader &reader, std::size_t index
               , const IStream::FilterInit &filterInit)
        : IStream(filterInit), pf_(reader.plug(index, fis_))
    {}

    virtual fs::path path() const { return pf_.path; }
    virtual void close() {}
    virtual boost::optional<std::size_t> size() const {
        return pf_.uncompressedSize;
    }

private:
    utility::zip::PluggedFile pf_;
};

boost::filesystem::path
findPrefix(const fs::path &path, const boost::optional<std::string> &hint
           , const utility::zip::Reader::Record::list &files)
{
    if (!hint) { return {}; }

    for (const auto &file : files) {
        if (file.path.filename() == *hint) {
            return file.path.parent_path();
        }
    }

    LOGTHROW(err2, std::runtime_error)
        << "No \"" << *hint << "\" found in the zip archive at "
        << path << ".";
    throw;
}

class Zip : public RoArchive::Detail {
public:
    Zip(const boost::filesystem::path &path
        , const boost::optional<std::string> &hint)
        : Detail(path), reader_(path)
        , prefix_(findPrefix(path, hint, reader_.files()))
    {
        for (const auto &file : reader_.files()) {
            if (!utility::isPathPrefix(file.path, prefix_)) { continue; }

            const auto path(utility::cutPathPrefix(file.path, prefix_));
            index_.insert(map::value_type(path, file.index));
        }
    }

    /** Get (wrapped) input stream for given file.
     *  Throws when not found.
     */
    virtual IStream::pointer istream(const boost::filesystem::path &path
                                     , const IStream::FilterInit &filterInit)
        const
    {
        auto findex(index_.find(path));
        if (findex == index_.end()) {
            LOGTHROW(err2, std::runtime_error)
                << "File " << path << " not found in the zip archive at "
                << path_ << ".";
        }

        return std::make_shared<ZipIStream>
            (reader_, findex->second, filterInit);
    }

    virtual bool exists(const boost::filesystem::path &path) const {
        return (index_.find(path) != index_.end());
    }

private:
    utility::zip::Reader reader_;
    const fs::path prefix_;
    typedef std::map<fs::path, std::size_t> map;
    map index_;
};

} // namespace

RoArchive::dpointer RoArchive::zip(const boost::filesystem::path &path
                                   , const boost::optional<std::string> &hint)
{
    return std::make_shared<Zip>(path, hint);
}

} // namespace roarchive
