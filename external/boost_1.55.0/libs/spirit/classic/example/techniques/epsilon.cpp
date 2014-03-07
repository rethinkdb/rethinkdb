/*=============================================================================
    Copyright (c) 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//
//  This example demonstrates the behaviour of epsilon_p when used as parser
//  generator.
//
//  The "r" is the rule, which is passed as a subject to the epsilon_p parser
//  generator. The "r" is the rule with binded semantic actions. But epsilon_p
//  parser generator is intended for looking forward and we don't want to
//  invoke semantic actions of subject parser. Hence the epsilon_p uses
//  the no_actions policy.
//
//  Because we want to use the "r" rule both in the epsilon_p and outside of it
//  we need the "r" to support two differant scanners, one with no_actions
//  action policy and one with the default action policy. To achieve this
//  we declare the "r" with the no_actions_scanner_list scanner type.
//
//-----------------------------------------------------------------------------
#define BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT 2

#include <boost/assert.hpp>
#include <iostream>
#include <boost/cstdlib.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/phoenix1.hpp>

using namespace std;
using namespace boost;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

//-----------------------------------------------------------------------------

int main()
{
    rule<
        // Support both the default phrase_scanner_t and the modified version
        // with no_actions action_policy
        no_actions_scanner_list<phrase_scanner_t>::type
    > r;

    int i(0);

    r = int_p[var(i) += arg1];

    parse_info<> info = parse(
        "1",

        // r rule is used twice but the semantic action is invoked only once
        epsilon_p(r) >> r,

        space_p
    );

    BOOST_ASSERT(info.full);
    // Check, that the semantic action was invoked only once
    BOOST_ASSERT(i == 1);

    return exit_success;
}

//-----------------------------------------------------------------------------
