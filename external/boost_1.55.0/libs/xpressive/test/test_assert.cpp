///////////////////////////////////////////////////////////////////////////////
// test_assert.cpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/xpressive/xpressive_static.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::xpressive;

bool three_or_six(ssub_match const &sub)
{
    return sub.length() == 3 || sub.length() == 6;
}

///////////////////////////////////////////////////////////////////////////////
// test1
//  simple custom assert that checks the length of a matched sub-expression
void test1()
{
    std::string str("foo barbaz fink");
    // match words of 3 characters or 6 characters.
    sregex rx = (bow >> +_w >> eow)[ check(&three_or_six) ] ;

    sregex_iterator first(str.begin(), str.end(), rx), last;
    BOOST_CHECK_EQUAL(std::distance(first, last), 2);
}

///////////////////////////////////////////////////////////////////////////////
// test2
//  same as above, but using a lambda
void test2()
{
    std::string str("foo barbaz fink");
    // match words of 3 characters or 6 characters.
    sregex rx = (bow >> +_w >> eow)[ check(length(_)==3 || length(_)==6) ] ;

    sregex_iterator first(str.begin(), str.end(), rx), last;
    BOOST_CHECK_EQUAL(std::distance(first, last), 2);
}

///////////////////////////////////////////////////////////////////////////////
// test3
//  more complicated use of custom assertions to validate a date
void test3()
{
    int const days_per_month[] =
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 31, 31};

    mark_tag month(1), day(2);
    // find a valid date of the form month/day/year.
    sregex date =
        (
            // Month must be between 1 and 12 inclusive
            (month= _d >> !_d)     [ check(as<int>(_) >= 1
                                        && as<int>(_) <= 12) ]
        >>  '/'
            // Day must be between 1 and 31 inclusive
        >>  (day=   _d >> !_d)     [ check(as<int>(_) >= 1
                                        && as<int>(_) <= 31) ]
        >>  '/'
            // Only consider years between 1970 and 2038
        >>  (_d >> _d >> _d >> _d) [ check(as<int>(_) >= 1970
                                        && as<int>(_) <= 2038) ]
        )
        // Ensure the month actually has that many days.
        [ check( ref(days_per_month)[as<int>(month)-1] >= as<int>(day) ) ]
    ;

    smatch what;
    std::string str("99/99/9999 2/30/2006 2/28/2006");

    BOOST_REQUIRE(regex_search(str, what, date));
    BOOST_CHECK_EQUAL(what[0], "2/28/2006");
}

using namespace boost::unit_test;

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test_assert");
    test->add(BOOST_TEST_CASE(&test1));
    test->add(BOOST_TEST_CASE(&test2));
    test->add(BOOST_TEST_CASE(&test3));
    return test;
}

