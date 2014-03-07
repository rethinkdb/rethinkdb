/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if defined(_MSC_VER)
# pragma warning(disable: 4180)     // qualifier applied to function type
                                    // has no meaning; ignored
#endif

// This tests the new behavior allowing attribute compatibility
// to semantic actions

#define BOOST_SPIRIT_ACTIONS_ALLOW_ATTR_COMPAT

#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <string>
#include "test.hpp"

void f(std::string const& s)
{
    std::cout << "parsing got: " << s << std::endl;
}

std::string s;
void b(char c)
{
    s += c;
}

int main()
{
    namespace qi = boost::spirit::qi;
    namespace phoenix = boost::phoenix;
    using spirit_test::test_attr;
    using spirit_test::test;

    {
        qi::rule<char const*, std::string()> r;
        r %= (+qi::char_)[phoenix::bind(&f, qi::_1)];

        std::string attr;
        BOOST_TEST(test_attr("abcdef", r, attr));
        BOOST_TEST(attr == "abcdef");
    }

    { // chaining
        using namespace boost::spirit::ascii;

        s = "";
        BOOST_TEST(test("a", char_[b][b]));
        BOOST_TEST(s == "aa");
    }

    return boost::report_errors();
}




