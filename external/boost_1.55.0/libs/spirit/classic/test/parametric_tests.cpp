/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c)      2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>
#include <string>

using namespace std;

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_parametric.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

#include <boost/detail/lightweight_test.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Parametric tests
//
///////////////////////////////////////////////////////////////////////////////

template <typename T>
static unsigned
length(T const *p)
{
    unsigned result = 0;
    while (*p++)
        ++result;
    return result;
}

template <typename T>
bool
is_equal(T const* a, T const* b)
{
    while (*a && *b)
        if (*a++ != *b++)
            return false;
    return true;
}

typedef rule< scanner<wchar_t const *> > wrule_t;

void
narrow_f_ch_p()
{
    char ch;
    rule<> r = anychar_p[var(ch) = arg1] >> *f_ch_p(const_(ch));
    parse_info<char const*> pi;

    pi = parse("aaaaaaaaa", r);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("aaaaabaaa", r);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(is_equal(pi.stop, "baaa"));
}

void
wide_f_ch_p()
{
    wchar_t ch;
    wrule_t r = anychar_p[var(ch) = arg1] >> *f_ch_p(const_(ch));
    parse_info<wchar_t const*> pi;

    pi = parse(L"aaaaaaaaa", r);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse(L"aaaaabaaa", r);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(is_equal(pi.stop, L"baaa"));
}

void
narrow_f_range_p()
{
    char from = 'a';
    char to = 'z';

    parse_info<char const*> pi;

    rule<> r2 = *f_range_p(const_(from), const_(to));
    pi = parse("abcdefghijklmnopqrstuvwxyz", r2);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("abcdefghijklmnopqrstuvwxyz123", r2);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(is_equal(pi.stop, "123"));
}

void
wide_f_range_p()
{
    wchar_t from = L'a';
    wchar_t to = L'z';

    parse_info<wchar_t const*> pi;

    wrule_t r2 = *f_range_p(const_(from), const_(to));
    pi = parse(L"abcdefghijklmnopqrstuvwxyz", r2);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse(L"abcdefghijklmnopqrstuvwxyz123", r2);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(is_equal(pi.stop, L"123"));
}

void
narrow_f_str_p()
{
    parse_info<char const*> pi;

    char const* start = "kim";
    char const* end = start + length(start);
    rule<> r3 = +f_str_p(const_(start), const_(end));

    pi = parse("kimkimkimkimkimkimkimkimkim", r3);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("kimkimkimkimkimkimkimkimkimmama", r3);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(is_equal(pi.stop, "mama"));

    pi = parse("joel", r3);
    BOOST_TEST(!pi.hit);
}

void
wide_f_str_p()
{
    parse_info<wchar_t const*> pi;

    wchar_t const* start = L"kim";
    wchar_t const* end = start + length(start);
    wrule_t r3 = +f_str_p(const_(start), const_(end));

    pi = parse(L"kimkimkimkimkimkimkimkimkim", r3);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse(L"kimkimkimkimkimkimkimkimkimmama", r3);
    BOOST_TEST(pi.hit);
    BOOST_TEST(!pi.full);
    BOOST_TEST(is_equal(pi.stop, L"mama"));

    pi = parse(L"joel", r3);
    BOOST_TEST(!pi.hit);
}

///////////////////////////////////////////////////////////////////////////////
//
//  test suite
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    narrow_f_ch_p();
    wide_f_ch_p();
    narrow_f_range_p();
    wide_f_range_p();
    narrow_f_str_p();
    wide_f_str_p();

    return boost::report_errors();
}

