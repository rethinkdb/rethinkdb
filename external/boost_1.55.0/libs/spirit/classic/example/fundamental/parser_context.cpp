/*=============================================================================
    Copyright (c) 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//
//  This example demonstrates usage of the parser_context template with
//  an explicit argument to declare rules with match results different from
//  nil_t. For better understanding, you should read the chapter "In-depth:
//  The Parser Context" in the documentation.
//
//  The default context of non-terminals is the parser_context.
//  The parser_context is a template with one argument AttrT, which is the type
//  of match attribute.
//
//  In this example int_rule is declared as rule with int match attribute's
//  type, so in int_rule variable we can hold any parser, which returns int
//  value. For example int_p or bin_p. And the most important is that we can
//  use returned value in the semantic action binded to the int_rule.
//
//-----------------------------------------------------------------------------
#include <iostream>
#include <boost/cstdlib.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/spirit/include/classic_core.hpp>

using namespace std;
using namespace boost;
using namespace phoenix;
using namespace BOOST_SPIRIT_CLASSIC_NS;

//-----------------------------------------------------------------------------

int main()
{
    rule<parser_context<int> > int_rule = int_p;

    parse(
        "123",
        // Using a returned value in the semantic action
        int_rule[cout << arg1 << endl]
    );

    return exit_success;
}

//-----------------------------------------------------------------------------

