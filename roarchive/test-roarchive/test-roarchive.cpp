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

#include <cstdlib>

#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include "dbglog/dbglog.hpp"
#include "roarchive/roarchive.hpp"

namespace bio = boost::iostreams;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        LOG(fatal) << "Missing parameters.";
        return EXIT_FAILURE;
    }

    roarchive::RoArchive archive(argv[1]);

    if (argc < 3) {
        for (const auto &path : archive.list()) {
            std::cout << path.string() << "\n";
        }
        std::cout.flush();
        return EXIT_SUCCESS;
    }

    // do we have any uncompressor?

    roarchive::IStream::FilterInit fi;
    if (argc > 3) {
        const std::string operation(argv[3]);
        if (operation == "gunzip") {
            fi = [](bio::filtering_istream &fis) {
                bio::zlib_params p;
                p.window_bits += 16;
                fis.push(bio::zlib_decompressor(p));
            };
        }
    }

    bio::copy(archive.istream(argv[2], fi)->get(), std::cout);

    return EXIT_SUCCESS;
}
