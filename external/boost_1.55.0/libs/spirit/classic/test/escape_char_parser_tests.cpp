/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_escape_char.hpp>

#include <iostream>
#include <boost/detail/lightweight_test.hpp>
#include <cstdio>       // for sprintf

#if !defined(BOOST_NO_CWCHAR) && !defined(BOOST_NO_SWPRINTF)
# include <cwchar>      // for swprintf
#endif

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    char c;

    // testing good C escapes
    BOOST_TEST(parse("a", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == 'a');
    BOOST_TEST(parse("\\b", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\b');
    BOOST_TEST(parse("\\t", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\t');
    BOOST_TEST(parse("\\n", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\n');
    BOOST_TEST(parse("\\f", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\f');
    BOOST_TEST(parse("\\r", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\r');
    BOOST_TEST(parse("\\\"", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\"');
    BOOST_TEST(parse("\\'", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\'');
    BOOST_TEST(parse("\\\\", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\\');
    BOOST_TEST(parse("\\120", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\120');
    BOOST_TEST(parse("\\x2e", c_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\x2e');

    // test bad C escapes
    BOOST_TEST(!parse("\\z", c_escape_ch_p[assign_a(c)]).hit);

    // testing good lex escapes
    BOOST_TEST(parse("a", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == 'a');
    BOOST_TEST(parse("\\b", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\b');
    BOOST_TEST(parse("\\t", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\t');
    BOOST_TEST(parse("\\n", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\n');
    BOOST_TEST(parse("\\f", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\f');
    BOOST_TEST(parse("\\r", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\r');
    BOOST_TEST(parse("\\\"", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\"');
    BOOST_TEST(parse("\\'", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\'');
    BOOST_TEST(parse("\\\\", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\\');
    BOOST_TEST(parse("\\120", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\120');
    BOOST_TEST(parse("\\x2e", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\x2e');
    BOOST_TEST(parse("\\z", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == 'z');
    BOOST_TEST(parse("\\a", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == 'a');

    // test bad lex escapes
    BOOST_TEST(!parse("\\xz", lex_escape_ch_p[assign_a(c)]).hit);

    // test out of range octal escape
    BOOST_TEST(!parse("\\777", lex_escape_ch_p[assign_a(c)]).hit);

#if CHAR_MAX == 127
    BOOST_TEST(!parse("\\200", lex_escape_ch_p[assign_a(c)]).hit);

    BOOST_TEST(parse("\\177", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\177');
#elif CHAR_MAX == 255
    BOOST_TEST(!parse("\\400", lex_escape_ch_p[assign_a(c)]).hit);

    BOOST_TEST(parse("\\377", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\377');
#endif

    // test out of range hex escape
    BOOST_TEST(!parse("\\xFFF", lex_escape_ch_p[assign_a(c)]).hit);

#if CHAR_MAX == 127
    BOOST_TEST(!parse("\\X80", lex_escape_ch_p[assign_a(c)]).hit);

    BOOST_TEST(parse("\\X7F", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\x7f');
#elif CHAR_MAX == 255
    BOOST_TEST(!parse("\\X100", lex_escape_ch_p[assign_a(c)]).hit);

    BOOST_TEST(parse("\\XFf", lex_escape_ch_p[assign_a(c)]).full);
    BOOST_TEST(c == '\xff');
#endif

#ifndef BOOST_NO_CWCHAR

    // test wide chars
    typedef escape_char_parser<lex_escapes, wchar_t> wlep_t;
    wlep_t wlep = wlep_t();

    typedef escape_char_parser<c_escapes, wchar_t> wcep_t;
    wcep_t wcep = wcep_t();

    //wchar_t const* wstr = L"a\\b\\t\\n\\f\\r\\\"\\'\\\\\\120\\x2e";
    //wchar_t const* wend(wstr + wcslen(wstr));

    wchar_t wc;
    BOOST_TEST(parse(L"a", wcep[assign_a(wc)]).hit);
    BOOST_TEST(wc == L'a');
    BOOST_TEST(parse(L"\\b", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\b');
    BOOST_TEST(parse(L"\\t", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\t');
    BOOST_TEST(parse(L"\\n", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\n');
    BOOST_TEST(parse(L"\\f", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\f');
    BOOST_TEST(parse(L"\\r", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\r');
    BOOST_TEST(parse(L"\\\"", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\"');
    BOOST_TEST(parse(L"\\'", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\'');
    BOOST_TEST(parse(L"\\\\", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\\');
    BOOST_TEST(parse(L"\\120", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\120');
    BOOST_TEST(parse(L"\\x2e", wcep[assign_a(wc)]).full);
    BOOST_TEST(wc == L'\x2e');

    // test bad wc escapes
    BOOST_TEST(!parse(L"\\z", wcep[assign_a(wc)]).hit);

    // test out of range octal escape
    size_t const octmax_size = 16;
    wchar_t octmax[octmax_size];

#if !defined(BOOST_NO_SWPRINTF)
    swprintf(octmax, octmax_size,
      L"\\%lo", (unsigned long)(std::numeric_limits<wchar_t>::max)());
    BOOST_TEST(parse(octmax, wlep[assign_a(wc)]).full);
    //BOOST_TEST(lex_escape_ch_p[assign_a(wc)].parse(str, end));
    BOOST_TEST(wc == (std::numeric_limits<wchar_t>::max)());

    swprintf(octmax, octmax_size,
      L"\\%lo", (unsigned long)(std::numeric_limits<wchar_t>::max)() + 1);
    BOOST_TEST(!parse(octmax, wlep[assign_a(wc)]).hit);

    // test out of range hex escape
    size_t const hexmax_size = 16;
    wchar_t hexmax[hexmax_size];

    swprintf(hexmax, hexmax_size,
      L"\\x%lx", (unsigned long)(std::numeric_limits<wchar_t>::max)());
    BOOST_TEST(parse(hexmax, wlep[assign_a(wc)]).full);
    BOOST_TEST(wc == (std::numeric_limits<wchar_t>::max)());

    swprintf(hexmax, hexmax_size,
      L"\\x%lx", (unsigned long)(std::numeric_limits<wchar_t>::max)() + 1);
    BOOST_TEST(!parse(hexmax, wlep[assign_a(wc)]).hit);
#endif // !defined(BOOST_NO_SWPRINTF)

#endif

    return boost::report_errors();
}
