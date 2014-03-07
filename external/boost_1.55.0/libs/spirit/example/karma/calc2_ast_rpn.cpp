/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A Calculator example demonstrating generation of AST which gets dumped into
//  a reverse polish notation afterwards.
//
//  [ JDG April 28, 2008 ]
//  [ HK April 28, 2008 ]
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/config/warning_disable.hpp>

#include <iostream>
#include <vector>
#include <string>

#include "calc2_ast.hpp"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

using namespace boost::spirit;
using namespace boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
//  Our calculator parser grammar
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct calculator 
  : qi::grammar<Iterator, expression_ast(), space_type>
{
    calculator() : calculator::base_type(expression)
    {
        expression =
            term                            [_val = _1]
            >> *(   ('+' >> term            [_val += _1])
                |   ('-' >> term            [_val -= _1])
                )
            ;

        term =
            factor                          [_val = _1]
            >> *(   ('*' >> factor          [_val *= _1])
                |   ('/' >> factor          [_val /= _1])
                )
            ;

        factor =
            uint_                           [_val = _1]
            |   '(' >> expression           [_val = _1] >> ')'
            |   ('-' >> factor              [_val = neg(_1)])
            |   ('+' >> factor              [_val = pos(_1)])
            ;
    }

    qi::rule<Iterator, expression_ast(), space_type> expression, term, factor;
};

// We need to tell fusion about our binary_op and unary_op structs
// to make them a first-class fusion citizen
//
// Note: we register the members exactly in the same sequence as we need them 
//       in the grammar
BOOST_FUSION_ADAPT_STRUCT(
    binary_op,
    (expression_ast, left)
    (expression_ast, right)
    (char, op)
)

BOOST_FUSION_ADAPT_STRUCT(
    unary_op,
    (expression_ast, right)
    (char, op)
)

///////////////////////////////////////////////////////////////////////////////
//  Our AST grammar for the generator, this prints the AST in reverse polish 
//  notation
///////////////////////////////////////////////////////////////////////////////
template <typename OuputIterator>
struct ast_rpn
  : karma::grammar<OuputIterator, expression_ast(), space_type>
{
    ast_rpn() : ast_rpn::base_type(ast_node)
    {
        ast_node %= int_ | binary_node | unary_node;
        binary_node %= ast_node << ast_node << char_;
        unary_node %= '(' << ast_node << char_ << ')';
    }

    karma::rule<OuputIterator, expression_ast(), space_type> ast_node;
    karma::rule<OuputIterator, binary_op(), space_type> binary_node;
    karma::rule<OuputIterator, unary_op(), space_type> unary_node;
};

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "RPN generator for simple expressions...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    //  Our parser grammar definitions
    typedef std::string::const_iterator iterator_type;
    typedef calculator<iterator_type> calculator;

    calculator calc; 

    // Our generator grammar definitions
    typedef std::back_insert_iterator<std::string> output_iterator_type;
    typedef ast_rpn<output_iterator_type> ast_rpn;

    ast_rpn ast_grammar;

    std::string str;
    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        expression_ast ast;   // this will hold the generated AST

        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = qi::phrase_parse(iter, end, calc, space, ast);

        if (r && iter == end)
        {
            std::string generated;
            output_iterator_type outit(generated);
            r = karma::generate_delimited(outit, ast_grammar, space, ast);

            if (r)
            {
                std::cout << "RPN for '" << str << "': \n" << generated 
                          << std::endl;
                std::cout << "-------------------------\n";
            }
            else
            {
                std::cout << "-------------------------\n";
                std::cout << "Generating failed\n";
                std::cout << "-------------------------\n";
            }
        }
        else
        {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \": " << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


