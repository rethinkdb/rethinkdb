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

#include <boost/spirit/include/classic_core.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Sub Rules tests
//
///////////////////////////////////////////////////////////////////////////////
void
subrules_tests()
{
    subrule<0>  start;
    subrule<1>  a;
    subrule<2>  b;
    subrule<3>  c;

    parse_info<char const*> pi;
    pi = parse("abcabcacb",
        (
            start   = *(a | b | c),
            a       = ch_p('a'),
            b       = ch_p('b'),
            c       = ch_p('c')
        )
    );

    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 9);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("aaaabababaaabbb",
        (
            start   = (a | b) >> (start | b),
            a       = ch_p('a'),
            b       = ch_p('b')
        )
    );

    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 15);
    BOOST_TEST(*pi.stop == 0);

    pi = parse("aaaabababaaabba",
        (
            start   = (a | b) >> (start | b),
            a       = ch_p('a'),
            b       = ch_p('b')
        )
    );

    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 14);

    pi = parse("aaaabababaaabbb",

        //  single subrule test
        start = (ch_p('a') | 'b') >> (start | 'b')
    );

    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 15);
    BOOST_TEST(*pi.stop == 0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    subrules_tests();
    return boost::report_errors();
}

