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

#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace roarchive {

/** Input stream.
 */
class IStream {
public:
    typedef std::shared_ptr<IStream> pointer;
    typedef std::function<void(boost::iostreams::filtering_istream&)
                          > FilterInit;

    IStream(const IStream::FilterInit &filterInit) {
        if (filterInit) { filterInit(fis_); }
    }

    virtual ~IStream() {}
    virtual boost::filesystem::path path() const = 0;
    virtual std::istream& get() { return fis_; }
    virtual void close() = 0;
    /** File size, if known.
     */
    virtual boost::optional<std::size_t> size() const { return boost::none; }

    operator std::istream&() { return get(); }

    /** Read whole file.
     */
    std::vector<char> read();

protected:
    boost::iostreams::filtering_istream fis_;
};

/** Generic read-only archive.
 *  One of plain directory, tarball or zip archive.
 *
 * Allows unified filesystem-like access to read-only data stored in various
 * standard formats.
 */
class RoArchive {
public:
    /** Opens read-only archive at given path
     *
     * Hint is name of filename known to the user. All access is adjusted to
     * subtree where known file is in the root of such subtree.
     *
     * Some implementations (tarball, zip find hint inside the archive. Others
     * check whether hint is under given path (plain directory).
     */
    RoArchive(const boost::filesystem::path &path
              , const boost::optional<std::string> &hint = boost::none);

    /** Checks file existence.
     */
    bool exists(const boost::filesystem::path &path) const;

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

    /** Internal implementation.
     */
    struct Detail;
    typedef std::shared_ptr<Detail> dpointer;

private:
    /** Path to archive.
     */
    boost::filesystem::path path_;

    /** Internal implementation.
     */
    dpointer detail_;
    Detail& detail() { return *detail_.get(); }
    const Detail& detail() const { return *detail_.get(); }

    /** Direct I/O flag
     */
    bool directio_;

    static dpointer directory(const boost::filesystem::path &path
                              , const boost::optional<std::string> &hint);
    static dpointer tarball(const boost::filesystem::path &path
                            , const boost::optional<std::string> &hint);
    static dpointer zip(const boost::filesystem::path &path
                        , const boost::optional<std::string> &hint);

    static dpointer factory(const boost::filesystem::path &path
                            , const boost::optional<std::string> &hint);
};

} // namespace roarchive

#endif // roarchive_roarchive_hpp_included_
