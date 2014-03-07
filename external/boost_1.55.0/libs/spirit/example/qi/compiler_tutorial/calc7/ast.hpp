/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CALC7_AST_HPP)
#define BOOST_SPIRIT_CALC7_AST_HPP

#include <boost/config/warning_disable.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <list>

namespace client { namespace ast
{
    ///////////////////////////////////////////////////////////////////////////
    //  The AST
    ///////////////////////////////////////////////////////////////////////////
    struct tagged
    {
        int id; // Used to annotate the AST with the iterator position.
                // This id is used as a key to a map<int, Iterator>
                // (not really part of the AST.)
    };

    struct nil {};
    struct signed_;
    struct expression;

    struct variable : tagged
    {
        variable(std::string const& name = "") : name(name) {}
        std::string name;
    };

    typedef boost::variant<
            nil
          , unsigned int
          , variable
          , boost::recursive_wrapper<signed_>
          , boost::recursive_wrapper<expression>
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

    struct expression
    {
        operand first;
        std::list<operation> rest;
    };

    struct assignment
    {
        variable lhs;
        expression rhs;
    };

    struct variable_declaration
    {
        assignment assign;
    };

    typedef boost::variant<
            variable_declaration
          , assignment>
    statement;

    typedef std::list<statement> statement_list;

    // print functions for debugging
    inline std::ostream& operator<<(std::ostream& out, nil) { out << "nil"; return out; }
    inline std::ostream& operator<<(std::ostream& out, variable const& var) { out << var.name; return out; }
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
    client::ast::expression,
    (client::ast::operand, first)
    (std::list<client::ast::operation>, rest)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::variable_declaration,
    (client::ast::assignment, assign)
)

BOOST_FUSION_ADAPT_STRUCT(
    client::ast::assignment,
    (client::ast::variable, lhs)
    (client::ast::expression, rhs)
)

#endif
