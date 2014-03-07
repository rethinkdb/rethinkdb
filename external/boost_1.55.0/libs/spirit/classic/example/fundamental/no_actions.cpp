/*=============================================================================
    Copyright (c) 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//
//  This example demonstrates no_actions_d directive.
//
//  The no_actions_d directive ensures, that semantic actions of the inner
//  parser would NOT be invoked. See the no_actions_scanner in the Scanner
//  and Parsing chapter in the User's Guide.
//
//-----------------------------------------------------------------------------

#include <boost/assert.hpp>
#include <iostream>
#include <boost/cstdlib.hpp>
#include <boost/spirit/include/classic_core.hpp>

using namespace std;
using namespace boost;
using namespace BOOST_SPIRIT_CLASSIC_NS;

//-----------------------------------------------------------------------------

int main()
{
    // To use the rule in the no_action_d directive we must declare it with
    // the no_actions_scanner scanner
    rule<no_actions_scanner<>::type> r;

    int i(0);

    // r is the rule with the semantic action
    r = int_p[assign_a(i)];

    parse_info<> info = parse(
        "1",

        no_actions_d
        [
            r
        ]
    );

    BOOST_ASSERT(info.full);
    // Check, that the action hasn't been invoked
    BOOST_ASSERT(i == 0);

    return exit_success;
}

//-----------------------------------------------------------------------------
