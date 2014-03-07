//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "test.hpp"

int main() 
{
    using spirit_test::test;
    using boost::spirit::qi::rule;
    using boost::spirit::qi::parse;
    using boost::spirit::qi::int_;
    using boost::spirit::qi::uint_;
    using boost::spirit::qi::short_;
    using boost::spirit::qi::ushort_;
    using boost::spirit::qi::long_;
    using boost::spirit::qi::ulong_;
    using boost::spirit::qi::float_;
    using boost::spirit::qi::double_;
    using boost::spirit::qi::long_double;
    using boost::spirit::qi::bool_;

#ifdef BOOST_HAS_LONG_LONG
    using boost::spirit::qi::long_long;
    using boost::spirit::qi::ulong_long;
#endif

    BOOST_TEST(test("-123", short_(0) | short_(-123)));
    BOOST_TEST(test("-123", short_(-123) | short_(0)));
  
    BOOST_TEST(test("123", ushort_(0) | ushort_(123)));
    BOOST_TEST(test("123", ushort_(123) | ushort_(0)));
  
    BOOST_TEST(test("-123456", int_(0) | int_(-123456)));
    BOOST_TEST(test("-123456", int_(-123456) | int_(0)));
  
    BOOST_TEST(test("123456", uint_(0) | uint_(123456)));
    BOOST_TEST(test("123456", uint_(123456) | uint_(0)));

    BOOST_TEST(test("-123456789", long_(0) | long_(-123456789L)));
    BOOST_TEST(test("-123456789", long_(-123456789L) | long_(0)));
  
    BOOST_TEST(test("123456789", ulong_(0) | ulong_(123456789UL)));
    BOOST_TEST(test("123456789", ulong_(123456789UL) | ulong_(0)));
  
#ifdef BOOST_HAS_LONG_LONG
    BOOST_TEST(test("-1234567890123456789",
        long_long(0) | long_long(-1234567890123456789LL)));
    BOOST_TEST(test("-1234567890123456789",
        long_long(-1234567890123456789LL) | long_long(0)));
  
    BOOST_TEST(test("1234567890123456789",
        ulong_long(0) | ulong_long(1234567890123456789ULL)));
    BOOST_TEST(test("1234567890123456789",
        ulong_long(1234567890123456789ULL) | ulong_long(0)));
#endif

    BOOST_TEST(test("1.23", float_(0) | float_(1.23f)));
    BOOST_TEST(test("1.23", float_(1.23f) | float_(0)));
    
    BOOST_TEST(test("123.456", double_(0) | double_(123.456)));
    BOOST_TEST(test("123.456", double_(123.456) | double_(0)));
    
    BOOST_TEST(test("123456.789", long_double(0) | long_double(123456.789l)));
    BOOST_TEST(test("123456.789", long_double(123456.789l) | long_double(0)));
    
    BOOST_TEST(test("true", bool_(false) | bool_(true)));
    BOOST_TEST(test("true", bool_(true) | bool_(false)));

    return boost::report_errors();
}

