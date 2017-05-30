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
    auto is(detail_->istream(path));
    // set exceptions
    is->get().exceptions(std::ios::badbit | std::ios::failbit);
    return is;
}

IStream::pointer RoArchive::istream(const boost::filesystem::path &path
                                    , const IStream::FilterInit &filterInit)
    const
{
    auto is(detail_->istream(path, filterInit));
    // set exceptions
    is->get().exceptions(std::ios::badbit | std::ios::failbit);
    return is;
}

bool RoArchive::exists(const boost::filesystem::path &path) const
{
    return detail_->exists(path);
}

boost::filesystem::path RoArchive::path(const boost::filesystem::path &path)
    const
{
    return path.is_absolute() ? path : (path_ / path);
}

std::vector<char> IStream::read()
{
    auto &s(get());
    std::vector<char> buf;
    if (const auto privided = size()) {
        buf.resize(*privided);
    } else {
        buf.resize(s.seekg(0, std::ios_base::end).tellg());
        s.seekg(0);
    }

    utility::binaryio::read(s, buf.data(), buf.size());
    return buf;
}

} // namespace roarchive
