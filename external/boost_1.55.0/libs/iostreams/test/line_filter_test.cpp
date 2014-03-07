// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cctype>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/line.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  
#include "detail/constants.hpp"
#include "detail/filters.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> // BCC 5.x.

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;    

struct toupper_line_filter : line_filter {
    std::string do_filter(const std::string& line)
    {
        std::string result(line);
        for ( std::string::size_type z = 0, len = line.size();
              z < len; 
              ++z )
        {
            result[z] = std::toupper((unsigned char) result[z]);
        }
        return result;
    }
};

bool compare_streams_in_lines(std::istream& first, std::istream& second)
{
    do {
        std::string line_one;
        std::string line_two;
        std::getline(first, line_one);
        std::getline(second, line_two);
        if (line_one != line_two || first.eof() != second.eof())
            return false;
    } while (!first.eof());
    return true;
}

void read_line_filter()
{
    test_file          src;
    uppercase_file     upper;
    filtering_istream  first;
    first.push(toupper_line_filter());
    first.push(file_source(src.name(), in_mode));
    ifstream second(upper.name().c_str(), in_mode);
    BOOST_CHECK_MESSAGE(
        compare_streams_in_lines(first, second),
        "failed reading from a line_filter"
    );
}

void write_line_filter() 
{
    test_file          data;
    temp_file          dest;
    uppercase_file     upper;

    filtering_ostream  out;
    out.push(toupper_line_filter());
    out.push(file_sink(dest.name(), out_mode));
    copy(file_source(data.name(), in_mode), out);
    out.reset();

    ifstream first(dest.name().c_str());
    ifstream second(upper.name().c_str());
    BOOST_CHECK_MESSAGE(
        compare_streams_in_lines(first, second),
        "failed writing to a line_filter"
    );
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("line_filter test");
    test->add(BOOST_TEST_CASE(&read_line_filter));
    test->add(BOOST_TEST_CASE(&write_line_filter));
    return test;
}

#include <boost/iostreams/detail/config/enable_warnings.hpp> // BCC 5.x.

