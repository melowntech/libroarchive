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
#ifndef roarchive_detail_hpp_included_
#define roarchive_detail_hpp_included_

#include <vector>

#include <boost/optional.hpp>

#include "utility/filesystem.hpp"

#include "roarchive.hpp"

namespace roarchive {

class RoArchive::Detail {
public:
    Detail(const boost::filesystem::path &path, bool directio = false)
        : path_(path), directio_(directio)
        , stat_(utility::FileStat::from(path, std::nothrow))
    {}

    virtual ~Detail() {}

    // Simple interface

    virtual IStream::pointer
    istream(const boost::filesystem::path &path
            , const IStream::FilterInit &filterInit) const = 0;

    IStream::pointer istream(const boost::filesystem::path &path) const {
        return istream(path, {});
    }

    /** Checks file existence.
     */
    virtual bool exists(const boost::filesystem::path &path) const = 0;

    virtual boost::optional<boost::filesystem::path>
    findFile(const std::string &filename) const = 0;

    /** List all files in the archive.
     */
    virtual std::vector<boost::filesystem::path> list() const = 0;

    virtual void applyHint(const FileHint &hint) = 0;

    bool changed() const;

    bool directio() const { return directio_; }

    virtual bool handlesSchema(const std::string&) const { return false; }

    const boost::filesystem::path &path() { return path_; }

    virtual const boost::optional<boost::filesystem::path>& usedHint() = 0;

protected:
    boost::filesystem::path path_;
    bool directio_;
    utility::FileStat stat_;
};

struct HintedPath {
    boost::filesystem::path path;
    boost::optional<boost::filesystem::path> usedHint;

    HintedPath(const boost::filesystem::path &path = boost::filesystem::path()
               , const boost::optional<boost::filesystem::path> &usedHint
               = boost::none)
        : path(path), usedHint(usedHint)
    {}

    operator const boost::filesystem::path&() const { return path; }
};

class FileHint::Matcher {
public:
    Matcher(const FileHint &hint)
        : hint_(hint.hint), bestIndex_(hint_.size())
    {}

    bool operator()(const boost::filesystem::path &path);
    operator bool() const { return bestIndex_ < hint_.size(); }
    bool operator!() const { return bestIndex_ == hint_.size(); }
    const boost::filesystem::path& match() const { return bestMatch_; }

private:
    const std::vector<std::string> hint_;
    std::size_t bestIndex_;
    boost::filesystem::path bestMatch_;
};

} // namespace roarchive

#endif // roarchive_detail_hpp_included_
