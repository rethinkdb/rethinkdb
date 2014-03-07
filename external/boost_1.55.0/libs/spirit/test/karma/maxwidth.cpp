//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_directive.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int 
main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    {
        BOOST_TEST(test("0123456789", maxwidth[lit("0123456789")]));
        BOOST_TEST(test("012345678", maxwidth[lit("012345678")]));
        BOOST_TEST(test("0123456789", maxwidth[lit("01234567890")]));

        BOOST_TEST(test("0123456789", maxwidth[string], "0123456789"));
        BOOST_TEST(test("012345678", maxwidth[string], "012345678"));
        BOOST_TEST(test("0123456789", maxwidth[string], "01234567890"));
    }

    {
        BOOST_TEST(test("01234567", maxwidth(8)[lit("01234567")]));
        BOOST_TEST(test("0123456", maxwidth(8)[lit("0123456")]));
        BOOST_TEST(test("01234567", maxwidth(8)[lit("012345678")]));

        BOOST_TEST(test("01234567", maxwidth(8)[string], "01234567"));
        BOOST_TEST(test("0123456", maxwidth(8)[string], "0123456"));
        BOOST_TEST(test("01234567", maxwidth(8)[string], "012345678"));
    }

    {
        std::string str;
        BOOST_TEST(test("01234567", 
            maxwidth(8, std::back_inserter(str))[lit("01234567")]) &&
            str.empty());

        str = "";
        BOOST_TEST(test("0123456", 
            maxwidth(8, std::back_inserter(str))[lit("0123456")]) &&
            str.empty());

        str = "";
        BOOST_TEST(test("01234567", 
            maxwidth(8, std::back_inserter(str))[lit("012345678")]) &&
            str == "8");
    }

    {
        using namespace boost::phoenix;

        BOOST_TEST(test("01234567", maxwidth(val(8))[lit("01234567")]));
        BOOST_TEST(test("0123456", maxwidth(val(8))[lit("0123456")]));
        BOOST_TEST(test("01234567", maxwidth(val(8))[lit("012345678")]));

        int w = 8;
        BOOST_TEST(test("01234567", maxwidth(ref(w))[string], "01234567"));
        BOOST_TEST(test("0123456", maxwidth(ref(w))[string], "0123456"));
        BOOST_TEST(test("01234567", maxwidth(ref(w))[string], "012345678"));
    }

    return boost::report_errors();
}
