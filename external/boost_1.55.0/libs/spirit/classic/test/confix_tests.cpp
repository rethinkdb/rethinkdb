/*=============================================================================
    Copyright (c) 2003 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace boost;
using namespace BOOST_SPIRIT_CLASSIC_NS;

typedef
    scanner<char const*, scanner_policies<skipper_iteration_policy<> > >
        scanner_t;

typedef
    rule<scanner_t>
        rule_t;

void comment_nest_p_test()
{
    rule_t r = comment_nest_p('{', '}');

    {
        parse_info<> info = parse("{a{b}c{d}e}", r, space_p);
        BOOST_TEST(info.full);
    }

    {
        parse_info<> info = parse("{a{b}c{d}e}x", r, space_p);
        BOOST_TEST(info.hit);
        BOOST_TEST(info.length == 11);
    }

    {
        char const* str = "x{a{b}c{d}e}";
        parse_info<> info = parse(str, r, space_p);
        BOOST_TEST(!info.hit);
        BOOST_TEST(info.stop == str);
    }

    {
        char const* str = "{a{b}c{d}e";
        parse_info<> info = parse(str, r, space_p);
        BOOST_TEST(!info.hit);
        BOOST_TEST(info.stop == (str + 10));
    }
}

int 
main()
{
    comment_nest_p_test();
    return boost::report_errors();
}

