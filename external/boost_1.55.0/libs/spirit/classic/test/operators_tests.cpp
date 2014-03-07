/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

using namespace std;

//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/classic_core.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Operators tests
//
///////////////////////////////////////////////////////////////////////////////
void
operators_tests()
{
    parse_info<char const*> pi;

    pi = parse("Hello World", str_p("Hello ") >> "World");
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 11);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("Banana", str_p("Banana") | "Pineapple");
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 6);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("Pineapple", str_p("Banana") | "Pineapple");
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 9);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("a.2  ", alpha_p || ('.' >> digit_p));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 3);

    pi = parse("a  ", alpha_p || ('.' >> digit_p));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse(".1  ", alpha_p || ('.' >> digit_p));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 2);

    pi = parse("1.a  ", alpha_p || ('.' >> digit_p));
    BOOST_TEST(!pi.hit);

    pi = parse("abcdefghij", +xdigit_p & +alpha_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 6);

    pi = parse("abcdefghij", +alpha_p & +xdigit_p);
    BOOST_TEST(!pi.hit);

    pi = parse("abcdefghij", +digit_p & +alpha_p);
    BOOST_TEST(!pi.hit);

    pi = parse("abcdefghij", +alpha_p & +digit_p);
    BOOST_TEST(!pi.hit);

    // Test for bug reported by Yusaku Sugai here:
    // http://article.gmane.org/gmane.comp.parsers.spirit.general/8544
    pi = parse( "foo", (anychar_p & anychar_p), ch_p(' ') );
    BOOST_TEST(pi.hit);

    pi = parse("F", xdigit_p - range_p('5','8'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("0", xdigit_p - range_p('5','8'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("4", xdigit_p - range_p('5','8'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("9", xdigit_p - range_p('5','8'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("5", xdigit_p - range_p('5','8'));
    BOOST_TEST(!pi.hit);

    pi = parse("7", xdigit_p - range_p('5','8'));
    BOOST_TEST(!pi.hit);

    pi = parse("x*/", anychar_p - "*/");
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("*/", anychar_p - "*/");
    BOOST_TEST(!pi.hit);

    pi = parse("switcher  ", str_p("switcher") - "switch");
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 8);

    pi = parse("Z", alpha_p ^ xdigit_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("1", alpha_p ^ xdigit_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("B", alpha_p ^ xdigit_p);
    BOOST_TEST(!pi.hit);

    pi = parse("B", !alpha_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 1);

    pi = parse("1", !alpha_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 0);
    BOOST_TEST(*pi.stop == '1');

    pi = parse("1234 5678 1234 5678", *(+digit_p >> *space_p));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 19);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("abcdefghijklmnopqrstuvwxyz123", *alpha_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.length == 26);

    pi = parse("1234+5678*1234/5678", +digit_p % (ch_p('+') | '*' | '/'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 19);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("1234+", +digit_p % '+');
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 4);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    operators_tests();
    return boost::report_errors();
}

