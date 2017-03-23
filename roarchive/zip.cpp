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
    ZipIStream(const utility::zip::Reader &reader, std::size_t index)
        : pf_(reader.plug(index, fis_))
    {
        fis_.exceptions(std::ios::badbit | std::ios::failbit);
    }

    virtual std::istream& get() { return fis_; }
    virtual fs::path path() const { return pf_.path; }
    virtual void close() {}
    virtual boost::optional<std::size_t> size() const {
        return pf_.uncompressedSize;
    }

private:
    bio::filtering_istream fis_;
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
    virtual IStream::pointer istream(const boost::filesystem::path &path) const
    {
        auto findex(index_.find(path));
        if (findex == index_.end()) {
            LOGTHROW(err2, std::runtime_error)
                << "File " << path << " not found in the zip archive at "
                << path_ << ".";
        }

        return std::make_shared<ZipIStream>(reader_, findex->second);
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
