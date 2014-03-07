/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test_attr;
    using boost::spirit::qi::no_skip;
    using boost::spirit::qi::lexeme;
    using boost::spirit::qi::char_;
    using boost::spirit::qi::space;

    // without skipping no_skip is equivalent to lexeme
    {
        std::string str;
        BOOST_TEST((test_attr("'  abc '", '\'' >> no_skip[+~char_('\'')] >> '\'', str) && 
            str == "  abc "));
    }
    {
        std::string str;
        BOOST_TEST((test_attr("'  abc '", '\'' >> lexeme[+~char_('\'')] >> '\'', str) && 
            str == "  abc "));
    }

    // with skipping, no_skip allows to match a leading skipper
    {
        std::string str;
        BOOST_TEST((test_attr("'  abc '", '\'' >> no_skip[+~char_('\'')] >> '\'', str, space) && 
            str == "  abc "));
    }
    {
        std::string str;
        BOOST_TEST((test_attr("'  abc '", '\'' >> lexeme[+~char_('\'')] >> '\'', str, space) && 
            str == "abc "));
    }

    return boost::report_errors();
}
