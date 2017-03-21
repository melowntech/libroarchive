#ifndef roarchive_detail_hpp_included_
#define roarchive_detail_hpp_included_

#include "./roarchive.hpp"

namespace roarchive {

class RoArchive::Detail {
public:
    Detail(const boost::filesystem::path &path
           , bool directio = false)
        : path_(path), directio_(directio)
    {}

    virtual ~Detail() {}

    /** Get (wrapped) input stream for given file.
     *  Throws when not found.
     */
    virtual IStream::pointer istream(const boost::filesystem::path &path)
        const = 0;

    bool directio() const { return directio_; }

protected:
    boost::filesystem::path path_;
    bool directio_;
};

} // namespace roarchive

#endif // roarchive_detail_hpp_included_

