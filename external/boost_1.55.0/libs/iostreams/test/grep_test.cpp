/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.

 * File:        libs/iostreams/test/grep_test.cpp
 * Date:        Mon May 26 17:48:45 MDT 2008
 * Copyright:   2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 *
 * Tests the class template basic_grep_filter.
 */

#include <iostream>

#include <boost/config.hpp>  // Make sure ptrdiff_t is in std.
#include <algorithm>
#include <cstddef>           // std::ptrdiff_t
#include <string>
#include <boost/iostreams/compose.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/grep.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/ref.hpp>
#include <boost/regex.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost;
using namespace boost::iostreams;
namespace io = boost::iostreams;
using boost::unit_test::test_suite;

// List of addresses of US Appeals Courts, from uscourts.gov
std::string addresses =
    "John Joseph Moakley United States Courthouse, Suite 2500\n"
    "One Courthouse Way\n"
    "Boston, MA 02210-3002\n"
    "\n"
    "Thurgood Marshall United States Courthouse, 18th Floor\n"
    "40 Centre Street\n"
    "New York, NY 10007-1501\n"
    "\n"
    "21400 James A. Byrne United States Courthouse\n"
    "601 Market Street\n"
    "Philadelphia, PA 19106-1729\n"
    "\n"
    "Lewis F. Powell, Jr. United States Courthouse Annex, Suite 501\n"
    "1100 East Main Street\n"
    "Richmond, VA 23219-3525\n"
    "\n"
    "F. Edward Hebert Federal Bldg\n"
    "600 South Maestri Place\n"
    "New Orleans, LA 70130\n"
    "\n"
    "Bob Casey United States Courthouse, 1st Floor\n"
    "515 Rusk Street\n"
    "Houston, TX 77002-2600\n"
    "\n"
    "Potter Stewart United States Courthouse, Suite 540\n"
    "100 East Fifth Street\n"
    "Cincinnati, OH 45202\n"
    "\n"
    "2722 Everett McKinley Dirksen United States Courthouse\n"
    "219 South Dearborn Street\n"
    "Chicago, IL 60604\n";

// Lines containing "United States Courthouse"
std::string us_courthouse =
    "John Joseph Moakley United States Courthouse, Suite 2500\n"
    "Thurgood Marshall United States Courthouse, 18th Floor\n"
    "21400 James A. Byrne United States Courthouse\n"
    "Lewis F. Powell, Jr. United States Courthouse Annex, Suite 501\n"
    "Bob Casey United States Courthouse, 1st Floor\n"
    "Potter Stewart United States Courthouse, Suite 540\n"
    "2722 Everett McKinley Dirksen United States Courthouse\n";

// Lines not containing "United States Courthouse"
std::string us_courthouse_inv = 
    "One Courthouse Way\n"
    "Boston, MA 02210-3002\n"
    "\n"
    "40 Centre Street\n"
    "New York, NY 10007-1501\n"
    "\n"
    "601 Market Street\n"
    "Philadelphia, PA 19106-1729\n"
    "\n"
    "1100 East Main Street\n"
    "Richmond, VA 23219-3525\n"
    "\n"
    "F. Edward Hebert Federal Bldg\n"
    "600 South Maestri Place\n"
    "New Orleans, LA 70130\n"
    "\n"
    "515 Rusk Street\n"
    "Houston, TX 77002-2600\n"
    "\n"
    "100 East Fifth Street\n"
    "Cincinnati, OH 45202\n"
    "\n"
    "219 South Dearborn Street\n"
    "Chicago, IL 60604\n";

// Lines containing a state and zip
std::string state_and_zip =
    "Boston, MA 02210-3002\n"
    "New York, NY 10007-1501\n"
    "Philadelphia, PA 19106-1729\n"
    "Richmond, VA 23219-3525\n"
    "New Orleans, LA 70130\n"
    "Houston, TX 77002-2600\n"
    "Cincinnati, OH 45202\n"
    "Chicago, IL 60604\n";

// Lines not containing a state and zip
std::string state_and_zip_inv =
    "John Joseph Moakley United States Courthouse, Suite 2500\n"
    "One Courthouse Way\n"
    "\n"
    "Thurgood Marshall United States Courthouse, 18th Floor\n"
    "40 Centre Street\n"
    "\n"
    "21400 James A. Byrne United States Courthouse\n"
    "601 Market Street\n"
    "\n"
    "Lewis F. Powell, Jr. United States Courthouse Annex, Suite 501\n"
    "1100 East Main Street\n"
    "\n"
    "F. Edward Hebert Federal Bldg\n"
    "600 South Maestri Place\n"
    "\n"
    "Bob Casey United States Courthouse, 1st Floor\n"
    "515 Rusk Street\n"
    "\n"
    "Potter Stewart United States Courthouse, Suite 540\n"
    "100 East Fifth Street\n"
    "\n"
    "2722 Everett McKinley Dirksen United States Courthouse\n"
    "219 South Dearborn Street\n";

// Lines containing at least three words
std::string three_words =
    "John Joseph Moakley United States Courthouse, Suite 2500\n"
    "One Courthouse Way\n"
    "Thurgood Marshall United States Courthouse, 18th Floor\n"
    "40 Centre Street\n"
    "21400 James A. Byrne United States Courthouse\n"
    "601 Market Street\n"
    "Lewis F. Powell, Jr. United States Courthouse Annex, Suite 501\n"
    "1100 East Main Street\n"
    "F. Edward Hebert Federal Bldg\n"
    "600 South Maestri Place\n"
    "Bob Casey United States Courthouse, 1st Floor\n"
    "515 Rusk Street\n"
    "Potter Stewart United States Courthouse, Suite 540\n"
    "100 East Fifth Street\n"
    "2722 Everett McKinley Dirksen United States Courthouse\n"
    "219 South Dearborn Street\n";

// Lines containing exactly three words
std::string exactly_three_words =
    "One Courthouse Way\n"
    "40 Centre Street\n"
    "601 Market Street\n"
    "515 Rusk Street\n";

// Lines that don't contain exactly three words
std::string exactly_three_words_inv =
    "John Joseph Moakley United States Courthouse, Suite 2500\n"
    "Boston, MA 02210-3002\n"
    "\n"
    "Thurgood Marshall United States Courthouse, 18th Floor\n"
    "New York, NY 10007-1501\n"
    "\n"
    "21400 James A. Byrne United States Courthouse\n"
    "Philadelphia, PA 19106-1729\n"
    "\n"
    "Lewis F. Powell, Jr. United States Courthouse Annex, Suite 501\n"
    "1100 East Main Street\n"
    "Richmond, VA 23219-3525\n"
    "\n"
    "F. Edward Hebert Federal Bldg\n"
    "600 South Maestri Place\n"
    "New Orleans, LA 70130\n"
    "\n"
    "Bob Casey United States Courthouse, 1st Floor\n"
    "Houston, TX 77002-2600\n"
    "\n"
    "Potter Stewart United States Courthouse, Suite 540\n"
    "100 East Fifth Street\n"
    "Cincinnati, OH 45202\n"
    "\n"
    "2722 Everett McKinley Dirksen United States Courthouse\n"
    "219 South Dearborn Street\n"
    "Chicago, IL 60604\n";

void test_filter( grep_filter grep, 
                  const std::string& input, 
                  const std::string& output );

void grep_filter_test()
{
    regex match_us_courthouse("\\bUnited States Courthouse\\b");
    regex match_state_and_zip("\\b[A-Z]{2}\\s+[0-9]{5}(-[0-9]{4})?\\b");
    regex match_three_words("\\b\\w+\\s+\\w+\\s+\\w+\\b");
    regex_constants::match_flag_type match_default = 
        regex_constants::match_default;

    {
        grep_filter grep(match_us_courthouse);
        test_filter(grep, addresses, us_courthouse);
    }

    {
        grep_filter grep(match_us_courthouse, match_default, grep::invert);
        test_filter(grep, addresses, us_courthouse_inv);
    }

    {
        grep_filter grep(match_state_and_zip);
        test_filter(grep, addresses, state_and_zip);
    }

    {
        grep_filter grep(match_state_and_zip, match_default, grep::invert);
        test_filter(grep, addresses, state_and_zip_inv);
    }

    {
        grep_filter grep(match_three_words);
        test_filter(grep, addresses, three_words);
    }

    {
        grep_filter grep(match_three_words, match_default, grep::whole_line);
        test_filter(grep, addresses, exactly_three_words);
    }

    {
        int options = grep::whole_line | grep::invert;
        grep_filter grep(match_three_words, match_default, options);
        test_filter(grep, addresses, exactly_three_words_inv);
    }
}

void test_filter( grep_filter grep, 
                  const std::string& input, 
                  const std::string& output )
{
    // Count lines in output
    std::ptrdiff_t count = std::count(output.begin(), output.end(), '\n');

    // Test as input filter
    {
        array_source  src(input.data(), input.data() + input.size());
        std::string   dest;
        io::copy(compose(boost::ref(grep), src), io::back_inserter(dest));
        BOOST_CHECK(dest == output);
        BOOST_CHECK(grep.count() == count);
    }

    // Test as output filter
    {
        array_source  src(input.data(), input.data() + input.size());
        std::string   dest;
        io::copy(src, compose(boost::ref(grep), io::back_inserter(dest)));
        BOOST_CHECK(dest == output);
        BOOST_CHECK(grep.count() == count);
    }
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("grep_filter test");
    test->add(BOOST_TEST_CASE(&grep_filter_test));
    return test;
}
