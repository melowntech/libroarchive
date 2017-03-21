#include "dbglog/dbglog.hpp"

#include "utility/magic.hpp"
#include "utility/binaryio.hpp"

#include "./roarchive.hpp"
#include "./detail.hpp"

namespace roarchive {

RoArchive::dpointer
RoArchive::factory(const boost::filesystem::path &path
                   , const boost::optional<std::string> &hint)
{
    const auto magic(utility::Magic().mime(path));

    if (magic == "inode/directory") { return directory(path, hint); }

    if (magic == "application/x-tar") { return tarball(path, hint); }

    if (magic == "application/zip") { return zip(path, hint); }

    LOGTHROW(err2, std::runtime_error)
        << "Unsupported archive type <" << magic << ">.";
    return {};
}

RoArchive::RoArchive(const boost::filesystem::path &path
                     , const boost::optional<std::string> &hint)
    : path_(path), detail_(factory(path, hint)), directio_(detail_->directio())
{}

IStream::pointer RoArchive::istream(const boost::filesystem::path &path) const
{
    return detail_->istream(path);
}

boost::filesystem::path RoArchive::path(const boost::filesystem::path &path)
    const
{
    return path.is_absolute() ? path : (path_ / path);
}

std::vector<char> IStream::read()
{
    auto &s(get());
    auto size(s.seekg(0, std::ios_base::end).tellg());
    s.seekg(0);

    std::vector<char> buf;
    buf.resize(size);
    utility::binaryio::read(s, buf.data(), buf.size());
    return buf;
}

} // namespace roarchive
