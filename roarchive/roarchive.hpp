#ifndef roarchive_roarchive_hpp_included_
#define roarchive_roarchive_hpp_included_

#include <iostream>
#include <memory>

#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>

namespace roarchive {

/** Input stream.
 */
class IStream {
public:
    typedef std::shared_ptr<IStream> pointer;
    virtual ~IStream() {}
    virtual boost::filesystem::path path() const = 0;
    virtual std::istream& get() = 0;
    virtual void close() = 0;

    operator std::istream&() { return get(); }

    /** Read whole file.
     */
    std::vector<char> read();
};

/** Generic read-only archive.
 *  Either plain directory or tarball (or zip archive in the future)
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
     * check whether hint is under given path (plan directory).
     */
    RoArchive(const boost::filesystem::path &path
              , const boost::optional<std::string> &hint = boost::none);

    /** Get input stream for file at given path.
     */
    IStream::pointer istream(const boost::filesystem::path &path) const;

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
