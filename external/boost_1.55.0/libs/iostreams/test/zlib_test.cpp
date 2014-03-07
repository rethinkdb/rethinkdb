// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <string>
#include <boost/iostreams/filter/test.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/sequence.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;     

struct zlib_alloc : std::allocator<char> { };

void zlib_test()
{
    text_sequence data;
    BOOST_CHECK(
        test_filter_pair( zlib_compressor(), 
                          zlib_decompressor(), 
                          std::string(data.begin(), data.end()) )
    );
    BOOST_CHECK(
        test_filter_pair( basic_zlib_compressor<zlib_alloc>(), 
                          basic_zlib_decompressor<zlib_alloc>(), 
                          std::string(data.begin(), data.end()) )
    );
    BOOST_CHECK(
        test_filter_pair( zlib_compressor(), 
                          zlib_decompressor(), 
                          std::string() )
    );
    {
        filtering_istream strm;
        strm.push( zlib_compressor() );
        strm.push( null_source() );
    }
    {
        filtering_istream strm;
        strm.push( zlib_decompressor() );
        strm.push( null_source() );
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("zlib test");
    test->add(BOOST_TEST_CASE(&zlib_test));
    return test;
}
