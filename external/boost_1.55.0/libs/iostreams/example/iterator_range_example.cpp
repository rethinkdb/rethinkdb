// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cassert>
#include <string>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/range/iterator_range.hpp>

namespace io = boost::iostreams;

int main()
{
    using namespace std;

    string                 input = "Hello World!";
    string                 output;
    io::filtering_istream  in(boost::make_iterator_range(input));
    getline(in, output);
    assert(input == output);
}
