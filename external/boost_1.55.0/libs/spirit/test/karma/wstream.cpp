//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cwchar>
#include <streambuf>
#include <iostream>

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/cstdint.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_stream.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include "test.hpp"

using namespace spirit_test;

// a simple complex number representation z = a + bi
struct complex
{
    complex (double a, double b)
      : a(a), b(b)
    {}

    double a;
    double b;

    template <typename Char>
    friend std::basic_ostream<Char>& 
    operator<< (std::basic_ostream<Char>& os, complex z)
    {
        os << "{" << z.a << "," << z.b << "}";
        return os;
    }
};

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;

    {
        BOOST_TEST(test(L"x", wstream, L'x'));
        BOOST_TEST(test(L"xyz", wstream, L"xyz"));
        BOOST_TEST(test(L"xyz", wstream, std::basic_string<wchar_t>(L"xyz")));
        BOOST_TEST(test(L"1", wstream, 1));
        BOOST_TEST(test(L"1.1", wstream, 1.1));
        BOOST_TEST(test(L"{1.2,2.4}", wstream, complex(1.2, 2.4)));
    }

    {
        BOOST_TEST(test(L"x", wstream(L'x')));
        BOOST_TEST(test(L"xyz", wstream(L"xyz")));
        BOOST_TEST(test(L"xyz", wstream(std::basic_string<wchar_t>(L"xyz"))));
        BOOST_TEST(test(L"1", wstream(1)));
        BOOST_TEST(test(L"1.1", wstream(1.1)));
        BOOST_TEST(test(L"{1.2,2.4}", wstream(complex(1.2, 2.4))));
    }

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test(L"x", lower[wstream], L'X'));
        BOOST_TEST(test(L"xyz", lower[wstream], L"XYZ"));
        BOOST_TEST(test(L"xyz", lower[wstream], std::basic_string<wchar_t>(L"XYZ")));
        BOOST_TEST(test(L"X", upper[wstream], L'x'));
        BOOST_TEST(test(L"XYZ", upper[wstream], L"xyz"));
        BOOST_TEST(test(L"XYZ", upper[wstream], std::basic_string<wchar_t>(L"xyz")));
    }

    {
        BOOST_TEST(test_delimited(L"x ", wstream, L'x', L' '));
        BOOST_TEST(test_delimited(L"xyz ", wstream, L"xyz", L' '));
        BOOST_TEST(test_delimited(L"xyz ", wstream, std::basic_string<wchar_t>(L"xyz"), L' '));
        BOOST_TEST(test_delimited(L"1 ", wstream, 1, ' '));
        BOOST_TEST(test_delimited(L"1.1 ", wstream, 1.1, ' '));
        BOOST_TEST(test_delimited(L"{1.2,2.4} ", wstream, complex(1.2, 2.4), ' '));
    }

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test_delimited(L"x ", lower[wstream], L'X', L' '));
        BOOST_TEST(test_delimited(L"xyz ", lower[wstream], L"XYZ", L' '));
        BOOST_TEST(test_delimited(L"xyz ", lower[wstream], std::basic_string<wchar_t>(L"XYZ"), L' '));
        BOOST_TEST(test_delimited(L"X ", upper[wstream], L'x', L' '));
        BOOST_TEST(test_delimited(L"XYZ ", upper[wstream], L"xyz", ' '));
        BOOST_TEST(test_delimited(L"XYZ ", upper[wstream], std::basic_string<wchar_t>(L"xyz"), L' '));
    }

    {   // lazy streams
        namespace phx = boost::phoenix;

        std::basic_string<wchar_t> ws(L"abc");
        BOOST_TEST((test(L"abc", wstream(phx::val(ws)))));
        BOOST_TEST((test(L"abc", wstream(phx::ref(ws)))));
    }

    {
        boost::optional<wchar_t> c;
        BOOST_TEST(!test(L"", wstream, c));
        c = L'x';
        BOOST_TEST(test(L"x", wstream, c));
    }

    return boost::report_errors();
}
