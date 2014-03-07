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
#include <boost/spirit/include/classic_loops.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Loops tests
//
///////////////////////////////////////////////////////////////////////////////
void
loops_tests()
{
    parse_info<char const*> pi;

    pi = parse("\"Hello World\"", "\"" >> *(anychar_p - "\"") >> "\"");
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 13);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("\"Hello World\"", "\"" >> repeat_p(0, more)[anychar_p - "\""] >> "\"");
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 13);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("xx", +ch_p('x'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 2);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("xx", repeat_p(1, more)[ch_p('x')]);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 2);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("", +ch_p('x'));
    BOOST_TEST(!pi.hit);

    pi = parse("", repeat_p(1, more)[ch_p('x')]);
    BOOST_TEST(!pi.hit);

    pi = parse("", *ch_p('x'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 0);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("", repeat_p(0, more)[ch_p('x')]);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 0);
    BOOST_TEST(*pi.stop == 0);

    //  repeat exact 8
    rule<>  rep8 = repeat_p(8)[alpha_p] >> 'X';
    BOOST_TEST(!parse("abcdefgX", rep8).hit);
    BOOST_TEST(parse("abcdefghX", rep8).full);
    BOOST_TEST(!parse("abcdefghiX", rep8).hit);
    BOOST_TEST(!parse("abcdefgX", rep8).hit);
    BOOST_TEST(!parse("aX", rep8).hit);

    //  repeat 2 to 8
    rule<>  rep28 = repeat_p(2, 8)[alpha_p] >> '*';
    BOOST_TEST(parse("abcdefg*", rep28).full);
    BOOST_TEST(parse("abcdefgh*", rep28).full);
    BOOST_TEST(!parse("abcdefghi*", rep28).hit);
    BOOST_TEST(!parse("a*", rep28).hit);

    //  repeat 2 or more
    rule<>  rep2_ = repeat_p(2, more)[alpha_p] >> '+';
    BOOST_TEST(parse("abcdefg+", rep2_).full);
    BOOST_TEST(parse("abcdefgh+", rep2_).full);
    BOOST_TEST(parse("abcdefghi+", rep2_).full);
    BOOST_TEST(parse("abcdefg+", rep2_).full);
    BOOST_TEST(!parse("a+", rep2_).hit);

    //  repeat 0
    rule<>  rep0 = repeat_p(0)[alpha_p] >> '/';
    BOOST_TEST(parse("/", rep0).full);
    BOOST_TEST(!parse("a/", rep0).hit);

    //  repeat 0 or 1
    rule<>  rep01 = repeat_p(0, 1)[alpha_p >> digit_p] >> '?';
    BOOST_TEST(!parse("abcdefg?", rep01).hit);
    BOOST_TEST(!parse("a?", rep01).hit);
    BOOST_TEST(!parse("1?", rep01).hit);
    BOOST_TEST(!parse("11?", rep01).hit);
    BOOST_TEST(!parse("aa?", rep01).hit);
    BOOST_TEST(parse("?", rep01).full);
    BOOST_TEST(parse("a1?", rep01).full);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    loops_tests();
    return boost::report_errors();
}

