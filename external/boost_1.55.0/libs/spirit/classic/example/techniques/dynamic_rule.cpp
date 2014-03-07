/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
// This example shows how the assign operator can be used to modify
// rules with semantic actions
// 
// First we show the basic spirit without (without any dynamic feature),
// then we show how to use assign_a to make it dynamic, 
//
// the grammar has to parse abcabc... sequences
///////////////////////////////////////////////////////////////////////////////
#include <iostream>

#define BOOST_SPIRIT_DEBUG
#include <boost/spirit.hpp>

#include <boost/spirit/include/classic_assign_actor.hpp>

int main(int argc, char* argv[])
{
    using namespace BOOST_SPIRIT_CLASSIC_NS;
    using namespace std;

    rule<> a,b,c,next;
    const char* str="abcabc";
    parse_info<> hit;
    BOOST_SPIRIT_DEBUG_NODE(next);
    BOOST_SPIRIT_DEBUG_NODE(a);
    BOOST_SPIRIT_DEBUG_NODE(b);
    BOOST_SPIRIT_DEBUG_NODE(c);
 
    // basic spirit gram
    a = ch_p('a') >> !b;
    b = ch_p('b') >> !c;
    c = ch_p('c') >> !a;

    hit = parse(str, a);    
    cout<<"hit :"<<( hit.hit ? "yes" : "no")<<", "
        <<(hit.full ? "full": "not full")
        <<endl;
    
    // using assign_a
    a = ch_p('a')[ assign_a( next, b)] >> !next;
    b = ch_p('b')[ assign_a( next, c)] >> !next;
    c = ch_p('c')[ assign_a( next, a)] >> !next;
    hit = parse(str, a);    
    cout<<"hit :"<<( hit.hit ? "yes" : "no")<<", "
        <<(hit.full ? "full": "not full")
        <<endl;

    return 0;
}
