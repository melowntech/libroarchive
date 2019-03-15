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
#ifndef roarchive_roarchive_hpp_included_
#define roarchive_roarchive_hpp_included_

#include <iostream>
#include <memory>
#include <functional>
#include <initializer_list>
#include <limits>

#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>

#include "istream.hpp"
#include "error.hpp"

namespace roarchive {

/** File hint. File that tells where actual root data directory starts.
 *  This is used for two things:
 *
 * * Solve problem with data being either directly at top level or in some
 *   unknown subridrectory.
 *
 * * Distinguish different file formats: therefore hint is an array not a
 *   scalar.
 *
 */
struct FileHint {
    FileHint() {}
    FileHint(std::string hint) : hint{ std::move(hint) } {}
    FileHint(std::vector<std::string> hint) : hint(std::move(hint)) {}

    template <typename T>
    FileHint(std::initializer_list<T> list)
        : hint(std::begin(list), std::end(list)) {}

    operator bool() const { return !hint.empty(); }

    std::vector<std::string> hint;

    class Matcher;
};

typedef std::vector<boost::filesystem::path> Files;

struct OpenOptions;

/** Generic read-only archive.
 *  One of plain directory, tarball or zip archive.
 *
 * Allows unified filesystem-like access to read-only data stored in various
 * standard formats.
 */
class RoArchive {
public:
    /** Opens read-only archive at given path.
     */
    RoArchive(const boost::filesystem::path &path);

    /** Opens read-only archive at given path.
     */
    RoArchive(const boost::filesystem::path &path
              , const OpenOptions &openOptions);

    /** Opens read-only archive at given path
     *
     * Hint is name of filename known to the user. All access is adjusted to
     * subtree where known file is in the root of such subtree.
     *
     * Some implementations (tarball, zip find hint inside the archive. Others
     * check whether hint is under given path (plain directory).
     *
     * Optional MIME type is used to detect file's format faster if known ahead.
     */
    RoArchive(const boost::filesystem::path &path
              , const FileHint &hint = FileHint()
              , const std::string &mime = "");

    /** Opens read-only archive at given path.
     *
     * File limit limits number of files * read from file list. This can be used
     * to analyze content of the archive when access bandwidth is scarce
     * (i.e. over network).
     *
     * Hint is name of filename known to the user. All access is adjusted to
     * subtree where known file is in the root of such subtree.
     *
     * Some implementations (tarball, zip find hint inside the archive. Others
     * check whether hint is under given path (plain directory).
     *
     * Optional MIME type is used to detect file's format faster if known ahead.
     */
    RoArchive(const boost::filesystem::path &path
              , std::size_t fileLimit
              , const FileHint &hint = FileHint()
              , const std::string &mime = "");

    /** Checks file existence.
     */
    bool exists(const boost::filesystem::path &path) const;

    /** Finds first occurence of given filename and returns full path
     */
    boost::optional<boost::filesystem::path>
    findFile(const std::string &filename) const;

    /** Get input stream for file at given path.
     */
    IStream::pointer istream(const boost::filesystem::path &path) const;

    /** Get input stream for file at given path.
     *  Internal filter is initialized by given init function.
     */
    IStream::pointer istream(const boost::filesystem::path &path
                             , const IStream::FilterInit &filterInit) const;

    /** Returns true in case of direct access to filesystem.
     *  Only directory "archive" supports this.
     *  Optimalization for direct file access.
     */
    bool directio() const { return directio_; }

    /** Returns path to file inside archive. Can be used to access file via
     *  directio.
     */
    boost::filesystem::path path(const boost::filesystem::path &path) const;

    /** List all files in the archive.
     */
    Files list() const;

    /** Post-constructor path hint application.
     */
    RoArchive& applyHint(const FileHint &hint = FileHint());

    /** Check for underlying data change.
     */
    bool changed() const;

    /** Internal implementation.
     */
    struct Detail;
    typedef std::shared_ptr<Detail> dpointer;

private:
    /** Internal implementation.
     */
    dpointer detail_;
    Detail& detail() { return *detail_.get(); }
    const Detail& detail() const { return *detail_.get(); }

    /** Direct I/O flag
     */
    bool directio_;

    static dpointer directory(const boost::filesystem::path &path
                              , const OpenOptions &openOptions);
    static dpointer tarball(const boost::filesystem::path &path
                            , const OpenOptions &openOptions);
    static dpointer zip(const boost::filesystem::path &path
                        , const OpenOptions &openOptions);

    static dpointer factory(boost::filesystem::path path
                            , OpenOptions openOptions);
};

/** Unifided open options;
 */
struct OpenOptions {
    FileHint hint;
    char inlineHint;
    std::size_t fileLimit;
    std::string mime;

    OpenOptions()
        : inlineHint(0)
        , fileLimit(std::numeric_limits<std::size_t>::max())
    {}

    OpenOptions& setHint(FileHint v) {
        hint = std::move(v); return *this;
    }

    OpenOptions& setInlineHint(char v) {
        inlineHint = v; return *this;
    }

    OpenOptions& setFileLimit(std::size_t v) {
        fileLimit = std::move(v); return *this;
    }

    OpenOptions& setMime(std::string v) {
        mime = std::move(v); return *this;
    }
};

} // namespace roarchive

#endif // roarchive_roarchive_hpp_included_
