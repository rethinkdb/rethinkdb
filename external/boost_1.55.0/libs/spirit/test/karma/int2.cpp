//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//#define KARMA_FAIL_COMPILATION

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_rule.hpp>
#include <boost/spirit/include/karma_phoenix_attributes.hpp>

#include <boost/limits.hpp>
#include "test.hpp"

using namespace spirit_test;


///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;

    {
        using namespace boost::spirit::ascii;

        karma::int_generator<int, 10, true> const signed_int =
            karma::int_generator<int, 10, true>();

        ///////////////////////////////////////////////////////////////////////
        BOOST_TEST(test(" 0", signed_int, 0));
        BOOST_TEST(test("+123", signed_int, 123));
        BOOST_TEST(test("-123", signed_int, -123));

        BOOST_TEST(test_delimited(" 0 ", signed_int, 0, char_(' ')));
        BOOST_TEST(test_delimited("+123 ", signed_int, 123, char_(' ')));
        BOOST_TEST(test_delimited("-123 ", signed_int, -123, char_(' ')));

        BOOST_TEST(test(" 0", lower[signed_int], 0));
        BOOST_TEST(test("+123", lower[signed_int], 123));
        BOOST_TEST(test("-123", lower[signed_int], -123));

        BOOST_TEST(test_delimited(" 0 ", lower[signed_int], 0, char_(' ')));
        BOOST_TEST(test_delimited("+123 ", lower[signed_int], 123, char_(' ')));
        BOOST_TEST(test_delimited("-123 ", lower[signed_int], -123, char_(' ')));

        BOOST_TEST(test(" 0", upper[signed_int], 0));
        BOOST_TEST(test("+123", upper[signed_int], 123));
        BOOST_TEST(test("-123", upper[signed_int], -123));

        BOOST_TEST(test_delimited(" 0 ", upper[signed_int], 0, char_(' ')));
        BOOST_TEST(test_delimited("+123 ", upper[signed_int], 123, char_(' ')));
        BOOST_TEST(test_delimited("-123 ", upper[signed_int], -123, char_(' ')));

        ///////////////////////////////////////////////////////////////////////
        BOOST_TEST(test(" 0", signed_int(0)));
        BOOST_TEST(test("+123", signed_int(123)));
        BOOST_TEST(test("-123", signed_int(-123)));

        BOOST_TEST(test_delimited(" 0 ", signed_int(0), char_(' ')));
        BOOST_TEST(test_delimited("+123 ", signed_int(123), char_(' ')));
        BOOST_TEST(test_delimited("-123 ", signed_int(-123), char_(' ')));

        BOOST_TEST(test(" 0", lower[signed_int(0)]));
        BOOST_TEST(test("+123", lower[signed_int(123)]));
        BOOST_TEST(test("-123", lower[signed_int(-123)]));

        BOOST_TEST(test_delimited(" 0 ", lower[signed_int(0)], char_(' ')));
        BOOST_TEST(test_delimited("+123 ", lower[signed_int(123)], char_(' ')));
        BOOST_TEST(test_delimited("-123 ", lower[signed_int(-123)], char_(' ')));

        BOOST_TEST(test(" 0", upper[signed_int(0)]));
        BOOST_TEST(test("+123", upper[signed_int(123)]));
        BOOST_TEST(test("-123", upper[signed_int(-123)]));

        BOOST_TEST(test_delimited(" 0 ", upper[signed_int(0)], char_(' ')));
        BOOST_TEST(test_delimited("+123 ", upper[signed_int(123)], char_(' ')));
        BOOST_TEST(test_delimited("-123 ", upper[signed_int(-123)], char_(' ')));

        using namespace boost::phoenix;

        BOOST_TEST(test(" 0", signed_int(val(0))));
        BOOST_TEST(test("+123", signed_int(val(123))));
        BOOST_TEST(test("-123", signed_int(val(-123))));

        int i1 = 0, i2 = 123, i3 = -123;
        BOOST_TEST(test(" 0", signed_int(ref(i1))));
        BOOST_TEST(test("+123", signed_int(ref(i2))));
        BOOST_TEST(test("-123", signed_int(ref(i3))));
    }

    {
        ///////////////////////////////////////////////////////////////////////
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("1234", uint_, 1234));
        BOOST_TEST(test("ff", hex, 0xff));
        BOOST_TEST(test("1234", oct, 01234));
        BOOST_TEST(test("11110000", bin, 0xf0));

        BOOST_TEST(test_delimited("1234 ", uint_, 1234, char_(' ')));
        BOOST_TEST(test_delimited("ff ", hex, 0xff, char_(' ')));
        BOOST_TEST(test_delimited("1234 ", oct, 01234, char_(' ')));
        BOOST_TEST(test_delimited("11110000 ", bin, 0xf0, char_(' ')));

        // test unsigned generator with signed integral value
        BOOST_TEST(test("ff", hex, (char)0xff));
        BOOST_TEST(test_delimited("ff ", hex, (char)0xff, char_(' ')));

        BOOST_TEST(test("1234", lower[uint_], 1234));
        BOOST_TEST(test("ff", lower[hex], 0xff));
        BOOST_TEST(test("1234", lower[oct], 01234));
        BOOST_TEST(test("11110000", lower[bin], 0xf0));

        BOOST_TEST(test_delimited("1234 ", lower[uint_], 1234, char_(' ')));
        BOOST_TEST(test_delimited("ff ", lower[hex], 0xff, char_(' ')));
        BOOST_TEST(test_delimited("1234 ", lower[oct], 01234, char_(' ')));
        BOOST_TEST(test_delimited("11110000 ", lower[bin], 0xf0, char_(' ')));

        BOOST_TEST(test("1234", upper[uint_], 1234));
        BOOST_TEST(test("FF", upper[hex], 0xff));
        BOOST_TEST(test("1234", upper[oct], 01234));
        BOOST_TEST(test("11110000", upper[bin], 0xf0));

        BOOST_TEST(test_delimited("1234 ", upper[uint_], 1234, char_(' ')));
        BOOST_TEST(test_delimited("FF ", upper[hex], 0xff, char_(' ')));
        BOOST_TEST(test_delimited("1234 ", upper[oct], 01234, char_(' ')));
        BOOST_TEST(test_delimited("11110000 ", upper[bin], 0xf0, char_(' ')));

        // no generator transformation should occur for uint_'s
        BOOST_TEST(test("1234", upper[upper[uint_]], 1234));
        BOOST_TEST(test("1234", upper[lower[uint_]], 1234));
        BOOST_TEST(test("1234", lower[upper[uint_]], 1234));
        BOOST_TEST(test("1234", lower[lower[uint_]], 1234));

        BOOST_TEST(test_delimited("1234 ", upper[upper[uint_]], 1234, char_(' ')));
        BOOST_TEST(test_delimited("1234 ", upper[lower[uint_]], 1234, char_(' ')));
        BOOST_TEST(test_delimited("1234 ", lower[upper[uint_]], 1234, char_(' ')));
        BOOST_TEST(test_delimited("1234 ", lower[lower[uint_]], 1234, char_(' ')));

        BOOST_TEST(test("FF", upper[upper[hex]], 0xff));
        BOOST_TEST(test("FF", upper[lower[hex]], 0xff));
        BOOST_TEST(test("ff", lower[upper[hex]], 0xff));
        BOOST_TEST(test("ff", lower[lower[hex]], 0xff));

        BOOST_TEST(test_delimited("FF ", upper[upper[hex]], 0xff, char_(' ')));
        BOOST_TEST(test_delimited("FF ", upper[lower[hex]], 0xff, char_(' ')));
        BOOST_TEST(test_delimited("ff ", lower[upper[hex]], 0xff, char_(' ')));
        BOOST_TEST(test_delimited("ff ", lower[lower[hex]], 0xff, char_(' ')));
    }

    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::karma::int_;
        using boost::spirit::karma::_1;
        using boost::spirit::karma::_val;
        using boost::spirit::karma::space;

        int i = 123;
        int j = 456;
        BOOST_TEST(test("123", int_[_1 = _val], i));
        BOOST_TEST(test_delimited("456 ", int_[_1 = _val], j, space));
    }

    return boost::report_errors();
}

