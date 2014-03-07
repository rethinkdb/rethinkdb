/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2009 Francois Barel

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A Calculator example demonstrating generation of AST which gets dumped into
//  a human readable format afterwards.
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
#include <boost/spirit/repository/include/karma_subrule.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

using namespace boost::spirit;
using namespace boost::spirit::ascii;
namespace repo = boost::spirit::repository;

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
    (char, op)
    (expression_ast, right)
)

BOOST_FUSION_ADAPT_STRUCT(
    unary_op,
    (char, op)
    (expression_ast, right)
)

///////////////////////////////////////////////////////////////////////////////
//  Our AST grammar for the generator, this just dumps the AST as a expression
///////////////////////////////////////////////////////////////////////////////
template <typename OuputIterator>
struct dump_ast
  : karma::grammar<OuputIterator, expression_ast(), space_type>
{
    dump_ast() : dump_ast::base_type(entry)
    {
        //[calc2_ast_dump_sr_def
        entry %= (
            ast_node %= int_ | binary_node | unary_node

          , binary_node %= '(' << ast_node << char_ << ast_node << ')'

          , unary_node %= '(' << char_ << ast_node << ')'
        );
        //]
    }

    karma::rule<OuputIterator, expression_ast(), space_type> entry;

    repo::karma::subrule<0, expression_ast()> ast_node;
    repo::karma::subrule<1, binary_op()> binary_node;
    repo::karma::subrule<2, unary_op()> unary_node;
};

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Dump AST's for simple expressions...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    //  Our parser grammar definitions
    typedef std::string::const_iterator iterator_type;
    typedef calculator<iterator_type> calculator;

    calculator calc; 

    // Our generator grammar definitions
    typedef std::back_insert_iterator<std::string> output_iterator_type;
    typedef dump_ast<output_iterator_type> dump_ast;

    dump_ast ast_grammar;

    std::string str;
    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        expression_ast ast;
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
                std::cout << "Got AST:" << std::endl << generated 
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


