// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cassert>
#include <string>
#include <boost/iostreams/stream.hpp>
#include <libs/iostreams/example/container_device.hpp>

namespace io = boost::iostreams;
namespace ex = boost::iostreams::example;

int main()
{
    using namespace std;
    typedef ex::container_source<string> string_source;

    string                     input = "Hello World!";
    string                     output;
    io::stream<string_source>  in(input);
    getline(in, output);
    assert(input == output);
}
