#include "dbglog/dbglog.hpp"

#include "utility/streams.hpp"

#include "./detail.hpp"

namespace fs = boost::filesystem;

namespace roarchive {

namespace {

class FileIStream : public IStream {
public:
    FileIStream(const fs::path &path)
        : path_(path)
        , stream_(path.string())
    {}

    virtual boost::filesystem::path path() const { return path_; }
    virtual std::istream& get() { return stream_; }
    virtual void close() { stream_.close(); }

private:
    const fs::path path_;
    utility::ifstreambuf stream_;
};

class Directory : public RoArchive::Detail {
public:
    Directory(const boost::filesystem::path &path
              , const boost::optional<std::string> &hint)
        : Detail(path, true)
    {
        // TODO: check for hint existence
        (void) hint;
    }

    /** Get (wrapped) input stream for given file.
     *  Throws when not found.
     */
    virtual IStream::pointer istream(const boost::filesystem::path &path) const
    {
        if (path.is_absolute()) {
            return std::make_shared<FileIStream>(path);
        }
        return std::make_shared<FileIStream>(path_ / path);
    }
};

} // namespace

RoArchive::dpointer
RoArchive::directory(const boost::filesystem::path &path
                     , const boost::optional<std::string> &hint)
{
    return std::make_shared<Directory>(path, hint);
}

} // namespace roarchive

