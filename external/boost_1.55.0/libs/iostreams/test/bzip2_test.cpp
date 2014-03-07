// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <string>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/sequence.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;  
namespace io = boost::iostreams;

struct bzip2_alloc : std::allocator<char> { };

void bzip2_test()
{
    text_sequence data;
    BOOST_CHECK(
        test_filter_pair( bzip2_compressor(), 
                          bzip2_decompressor(), 
                          std::string(data.begin(), data.end()) )
    );
    BOOST_CHECK(
        test_filter_pair( basic_bzip2_compressor<bzip2_alloc>(), 
                          basic_bzip2_decompressor<bzip2_alloc>(), 
                          std::string(data.begin(), data.end()) )
    );
    BOOST_CHECK(
        test_filter_pair( bzip2_compressor(), 
                          bzip2_decompressor(), 
                          std::string() )
    );
    {
        filtering_istream strm;
        strm.push( bzip2_compressor() );
        strm.push( null_source() );
    }
    {
        filtering_istream strm;
        strm.push( bzip2_decompressor() );
        strm.push( null_source() );
    }
}    

void multiple_member_test()
{
    text_sequence      data;
    std::vector<char>  temp, dest;

    // Write compressed data to temp, twice in succession
    filtering_ostream out;
    out.push(bzip2_compressor());
    out.push(io::back_inserter(temp));
    io::copy(make_iterator_range(data), out);
    out.push(io::back_inserter(temp));
    io::copy(make_iterator_range(data), out);

    // Read compressed data from temp into dest
    filtering_istream in;
    in.push(bzip2_decompressor());
    in.push(array_source(&temp[0], temp.size()));
    io::copy(in, io::back_inserter(dest));

    // Check that dest consists of two copies of data
    BOOST_REQUIRE_EQUAL(data.size() * 2, dest.size());
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin()));
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin() + dest.size() / 2));

    dest.clear();
    io::copy(
        array_source(&temp[0], temp.size()),
        io::compose(bzip2_decompressor(), io::back_inserter(dest)));

    // Check that dest consists of two copies of data
    BOOST_REQUIRE_EQUAL(data.size() * 2, dest.size());
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin()));
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin() + dest.size() / 2));
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("bzip2 test");
    test->add(BOOST_TEST_CASE(&bzip2_test));
    test->add(BOOST_TEST_CASE(&multiple_member_test));
    return test;
}
