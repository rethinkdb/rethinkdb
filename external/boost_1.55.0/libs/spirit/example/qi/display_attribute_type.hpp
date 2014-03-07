//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example implements a simple utility allowing to print the attribute 
//  type as it is exposed by an arbitrary Qi parser expression. Just insert 
//  your expression below, compile and run this example to see what Qi is 
//  seeing!

#if !defined (DISPLAY_ATTRIBUTE_OF_PARSER_JAN_2010_30_0722PM)
#define DISPLAY_ATTRIBUTE_OF_PARSER_JAN_2010_30_0722PM

#include <iostream>
#include <boost/spirit/include/qi.hpp>

namespace tools
{
    namespace spirit = boost::spirit;

    template <typename Expr, typename Iterator = spirit::unused_type>
    struct attribute_of_parser
    {
        typedef typename spirit::result_of::compile<
            spirit::qi::domain, Expr
        >::type parser_expression_type;

        typedef typename spirit::traits::attribute_of<
            parser_expression_type, spirit::unused_type, Iterator
        >::type type;
    };

    template <typename T>
    void display_attribute_of_parser(T const &) 
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, T);

        typedef typename attribute_of_parser<T>::type type;
        std::cout << typeid(type).name() << std::endl;
    }

    template <typename T>
    void display_attribute_of_parser(std::ostream& os, T const &) 
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, T);

        typedef typename attribute_of_parser<T>::type type;
        os << typeid(type).name() << std::endl;
    }
}

#endif

