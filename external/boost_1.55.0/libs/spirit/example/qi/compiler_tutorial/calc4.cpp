/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  A Calculator example demonstrating generation of AST. The AST,
//  once created, is traversed, 1) To print its contents and
//  2) To evaluate the result.
//
//  [ JDG April 28, 2008 ]      For BoostCon 2008
//  [ JDG February 18, 2011 ]   Pure attributes. No semantic actions.
//
///////////////////////////////////////////////////////////////////////////////

// Spirit v2.5 allows you to suppress automatic generation
// of predefined terminals to speed up complation. With
// BOOST_SPIRIT_NO_PREDEFINED_TERMINALS defined, you are
// responsible in creating instances of the terminals that
// you need (e.g. see qi::uint_type uint_ below).
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#if defined(_MSC_VER)
# pragma warning(disable: 4345)
#endif

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <string>

namespace client { namespace ast
{
    ///////////////////////////////////////////////////////////////////////////
    //  The AST
    ///////////////////////////////////////////////////////////////////////////
    struct nil {};
    struct signed_;
    struct program;

    typedef boost::variant<
            nil
          , unsigned int
          , boost::recursive_wrapper<signed_>
          , boost::recursive_wrapper<program>
        >
    operand;

    struct signed_
    {
        char sign;
        operand operand_;
    };

    struct operation
    {
        char operator_;
        operand operand_;
    };

    struct program
    {
        operand first;
        std::list<operation> rest;
    };
}}

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::signed_,
    (char, sign)
    (client::ast::operand, operand_)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::operation,
    (char, operator_)
    (client::ast::operand, operand_)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::program,
    (client::ast::operand, first)
    (std::list<client::ast::operation>, rest)
)

namespace client { namespace ast
{
    ///////////////////////////////////////////////////////////////////////////
    //  The AST Printer
    ///////////////////////////////////////////////////////////////////////////
    struct printer
    {
        typedef void result_type;

        void operator()(nil) const {}
        void operator()(unsigned int n) const { std::cout << n; }

        void operator()(operation const& x) const
        {
            boost::apply_visitor(*this, x.operand_);
            switch (x.operator_)
            {
                case '+': std::cout << " add"; break;
                case '-': std::cout << " subt"; break;
                case '*': std::cout << " mult"; break;
                case '/': std::cout << " div"; break;
            }
        }

        void operator()(signed_ const& x) const
        {
            boost::apply_visitor(*this, x.operand_);
            switch (x.sign)
            {
                case '-': std::cout << " neg"; break;
                case '+': std::cout << " pos"; break;
            }
        }

        void operator()(program const& x) const
        {
            boost::apply_visitor(*this, x.first);
            BOOST_FOREACH(operation const& oper, x.rest)
            {
                std::cout << ' ';
                (*this)(oper);
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The AST evaluator
    ///////////////////////////////////////////////////////////////////////////
    struct eval
    {
        typedef int result_type;

        int operator()(nil) const { BOOST_ASSERT(0); return 0; }
        int operator()(unsigned int n) const { return n; }

        int operator()(operation const& x, int lhs) const
        {
            int rhs = boost::apply_visitor(*this, x.operand_);
            switch (x.operator_)
            {
                case '+': return lhs + rhs;
                case '-': return lhs - rhs;
                case '*': return lhs * rhs;
                case '/': return lhs / rhs;
            }
            BOOST_ASSERT(0);
            return 0;
        }

        int operator()(signed_ const& x) const
        {
            int rhs = boost::apply_visitor(*this, x.operand_);
            switch (x.sign)
            {
                case '-': return -rhs;
                case '+': return +rhs;
            }
            BOOST_ASSERT(0);
            return 0;
        }

        int operator()(program const& x) const
        {
            int state = boost::apply_visitor(*this, x.first);
            BOOST_FOREACH(operation const& oper, x.rest)
            {
                state = (*this)(oper, state);
            }
            return state;
        }
    };
}}

namespace client
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    ///////////////////////////////////////////////////////////////////////////////
    //  The calculator grammar
    ///////////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct calculator : qi::grammar<Iterator, ast::program(), ascii::space_type>
    {
        calculator() : calculator::base_type(expression)
        {
            qi::uint_type uint_;
            qi::char_type char_;

            expression =
                term
                >> *(   (char_('+') >> term)
                    |   (char_('-') >> term)
                    )
                ;

            term =
                factor
                >> *(   (char_('*') >> factor)
                    |   (char_('/') >> factor)
                    )
                ;

            factor =
                    uint_
                |   '(' >> expression >> ')'
                |   (char_('-') >> factor)
                |   (char_('+') >> factor)
                ;
        }

        qi::rule<Iterator, ast::program(), ascii::space_type> expression;
        qi::rule<Iterator, ast::program(), ascii::space_type> term;
        qi::rule<Iterator, ast::operand(), ascii::space_type> factor;
    };
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;
    typedef client::calculator<iterator_type> calculator;
    typedef client::ast::program ast_program;
    typedef client::ast::printer ast_print;
    typedef client::ast::eval ast_eval;

    std::string str;
    while (std::getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        calculator calc;        // Our grammar
        ast_program program;    // Our program (AST)
        ast_print print;        // Prints the program
        ast_eval eval;          // Evaluates the program

        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        boost::spirit::ascii::space_type space;
        bool r = phrase_parse(iter, end, calc, space, program);

        if (r && iter == end)
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            print(program);
            std::cout << "\nResult: " << eval(program) << std::endl;
            std::cout << "-------------------------\n";
        }
        else
        {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \" " << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}


