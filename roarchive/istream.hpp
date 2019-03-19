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
#ifndef roarchive_istream_hpp_included_
#define roarchive_istream_hpp_included_

#include <ctime>
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
    typedef std::unique_ptr<IStream> pointer;
    typedef std::function<void(boost::iostreams::filtering_istream&)
                          > FilterInit;

    IStream(const IStream::FilterInit &filterInit
            , const boost::optional<std::size_t> &size = boost::none
            , bool seekable = true
            , std::time_t timestamp = -1)
        : stacked_(false), seekable_(seekable), timestamp_(timestamp)
    {
        if (filterInit) { filterInit(fis_); }
        if (!fis_.size()) {
            size_ = size;
        } else {
            // stacked: we cannot assume anything, stay on the safe side
            stacked_ = true;
            seekable_ = false;
        }
    }

    virtual ~IStream() {}

    /** Real full path inside the archive.
     */
    virtual boost::filesystem::path path() const = 0;

    /** Path this stream was obtained by from the index.
     */
    virtual boost::filesystem::path index() const = 0;

    virtual std::istream& get() { return fis_; }
    virtual void close() = 0;

    /** File size, if known.
     */
    boost::optional<std::size_t> size() const { return size_; }

    /** File timestamp. Value < 0 marks now.
     */
    std::time_t timestamp() const { return timestamp_; }

    bool seekable() const { return seekable_; }

    operator std::istream&() { return get(); }

    /** Read whole file. File must not be read from before.
     */
    std::vector<char> read();

protected:
    void update(const boost::optional<std::size_t> &size = boost::none
                , bool seekable = true)
    {
        if (!stacked_) {
            size_ = size;
            seekable_ = seekable;
        }
    }

    boost::iostreams::filtering_istream fis_;

private:
    bool stacked_;
    bool seekable_;
    boost::optional<std::size_t> size_;
    std::time_t timestamp_;
};

// support operations

/** Copies open input stream to C++ ostream.
 */
void copy(const IStream::pointer &in, std::ostream &out);

/** Copies open input stream to local file.
 */
void copy(const IStream::pointer &in, const boost::filesystem::path &out);

} // namespace roarchive

#endif // roarchive_istream_hpp_included_
