#include "dbglog/dbglog.hpp"

#include "utility/streams.hpp"

#include "./detail.hpp"

namespace roarchive {

namespace {

} // namespace

RoArchive::dpointer RoArchive::zip(const boost::filesystem::path &path
                                   , const boost::optional<std::string> &hint)
{
    LOGTHROW(err2, std::runtime_error)
        << "roarchive: zip support unimplemented, yet.";
    return {};
    (void) path;
    (void) hint;
}

} // namespace roarchive
