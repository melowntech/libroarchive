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
#include <iostream>

#include <boost/filesystem.hpp>

#include "utility/buildsys.hpp"
#include "utility/gccversion.hpp"
#include "utility/streams.hpp"

#include "service/cmdline.hpp"

#include "roarchive/roarchive.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace {

class Cat : public service::Cmdline
{
public:
    Cat()
        : service::Cmdline("roarchive-cat", BUILD_TARGET_VERSION)
    {}

private:
    virtual void configuration(po::options_description &cmdline
                               , po::options_description &config
                               , po::positional_options_description &pd)
        UTILITY_OVERRIDE;

    virtual void configure(const po::variables_map &vars)
        UTILITY_OVERRIDE;

    virtual bool help(std::ostream &out, const std::string &what) const
        UTILITY_OVERRIDE;

    virtual int run() UTILITY_OVERRIDE;

    fs::path archive_;
    fs::path filename_;
};

void Cat::configuration(po::options_description &cmdline
                        , po::options_description &config
                        , po::positional_options_description &pd)
{
    cmdline.add_options()
        ("archive", po::value(&archive_)->required()
         , "Archive to open.")
        ("filename", po::value(&filename_)->required()
         , "Name of file to extract.")
        ;

    pd.add("archive", 1)
        .add("filename", 1);

    (void) config;
}

void Cat::configure(const po::variables_map &vars)
{
    (void) vars;
}

bool Cat::help(std::ostream &out, const std::string &what) const
{
    if (what.empty()) {
        out << R"RAW(roarchive-cat
usage
    vef2vts ARCHIVE FILENAME [OPTIONS]

)RAW";
    }
    return false;
}

int Cat::run()
{
    roarchive::RoArchive archive(archive_);

    std::cout << archive.istream(filename_)->get().rdbuf();
    std::cout.flush();

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char *argv[])
{
    return Cat()(argc, argv);
}
