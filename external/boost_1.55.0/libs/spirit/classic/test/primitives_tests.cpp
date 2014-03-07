/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>

using namespace std;
#include <boost/spirit/include/classic_core.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "impl/string_length.hpp"
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  Primitives tests
//
///////////////////////////////////////////////////////////////////////////////
void
primitives_tests()
{
    char const* cp = "xyz.Jambalaya";
    char const* cp_first = cp;
    char const* cp_last = cp + test_impl::string_length(cp);

    chlit<> cpx('x');
    parse_info<char const*> pi = parse(cp_first, cp_last, cpx);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 1);
    BOOST_TEST(pi.stop == &cp[1]);

    pi = parse(pi.stop, ch_p('y'));
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 1);
    BOOST_TEST(pi.stop == &cp[2]);

    scanner<char const*> scan(pi.stop, cp_last);
    match<char> hit = ch_p('z').parse(scan);
    BOOST_TEST(hit);
    BOOST_TEST(hit.value() == 'z');
    BOOST_TEST(pi.stop == &cp[3]);

    pi = parse(pi.stop, eps_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 0);
    BOOST_TEST(pi.stop == &cp[3]);

    pi = parse(pi.stop, nothing_p);
    BOOST_TEST(!pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.stop == &cp[3]);

    pi = parse(pi.stop, anychar_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(pi.length == 1);
    BOOST_TEST(pi.stop == &cp[4]);

    scan.first = pi.stop;
    hit = range_p('A','Z').parse(scan);
    BOOST_TEST(hit);
    BOOST_TEST(hit.value() == 'J');
    BOOST_TEST(pi.stop == &cp[5]);

    strlit<char const*>     sp("ambalaya");
    strlit<wchar_t const*>  wsp(L"ambalaya");

    char const* save = pi.stop;
    pi = parse(save, sp);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 8);
    BOOST_TEST(pi.stop == cp_last);

    pi = parse(save, wsp);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(pi.length == 8);
    BOOST_TEST(pi.stop == cp_last);

    pi = parse("\n", eol_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("\r", eol_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("\r\n", eol_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("\n\r", eol_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);

    pi = parse("", end_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("1", ~alpha_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("a", ~alpha_p);
    BOOST_TEST(!pi.hit);

    pi = parse("a", ~~alpha_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("1", ~~alpha_p);
    BOOST_TEST(!pi.hit);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    primitives_tests();
    if (boost::report_errors() == 0)
        cout << "Tests concluded successfully\n";
    return boost::report_errors();
}

