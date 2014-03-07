//
//  Copyright (c) 2004 Joao Abecasis
//
//  Use, modification and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/spirit/include/classic_core.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

void shortest_alternative_parser_test()
{
    typedef
        shortest_alternative<
            shortest_alternative<
                shortest_alternative<
                    strlit<>,
                    strlit<> >,
                strlit<> >,
            strlit<> >
    parser_t;

    parser_t short_rule =
        shortest_d[
            str_p("a")
            | str_p("aa")
            | str_p("aaa")
            | str_p("aaaa")
        ];

    BOOST_TEST(parse("a", short_rule).full);
    BOOST_TEST(parse("aa", short_rule).length == 1);
    BOOST_TEST(parse("aaa", short_rule).length == 1);
    BOOST_TEST(parse("aaaa", short_rule).length == 1);

    short_rule =
        shortest_d[
            str_p("d")
            | str_p("cd")
            | str_p("bcd")
            | str_p("abcd")
        ];

    BOOST_TEST(parse("d", short_rule).full);
    BOOST_TEST(parse("cd", short_rule).full);
    BOOST_TEST(parse("bcd", short_rule).full);
    BOOST_TEST(parse("abcd", short_rule).full);
}

int 
main()
{
    shortest_alternative_parser_test();
    return boost::report_errors();
}
