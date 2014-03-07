/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

///////////////////////////////////////////////////////////////////////////////
// Test suite for push_front_actor
///////////////////////////////////////////////////////////////////////////////

#include "action_tests.hpp"
#include <string>
#include <vector>
#include <deque>
#include <cstring>
#include <iostream>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_push_front_actor.hpp>
#include <boost/spirit/include/classic_lists.hpp>

template<typename ContainerT>
void push_front_test()
{
    using namespace BOOST_SPIRIT_CLASSIC_NS;

    const char* cp = "one,two,three";
    const char* cp_first = cp;
    const char* cp_last = cp + test_impl::string_length(cp);
    const char* cp_i[] = {"one","two","three"};;
    int i;
    ContainerT c;
    typename ContainerT::const_iterator it;

    scanner<char const*> scan( cp_first, cp_last );
    match<> hit;

    hit = list_p( (*alpha_p)[ push_front_a(c)] , ch_p(',') ).parse(scan);
    BOOST_CHECK(hit);
    BOOST_CHECK_EQUAL(scan.first, scan.last);
    BOOST_CHECK_EQUAL( c.size(), static_cast<typename ContainerT::size_type>(3));
    for (i=2, it = c.begin();i>=0 && it != c.end();--i, ++it)
        BOOST_CHECK_EQUAL( cp_i[i], *it);
    scan.first = cp;
}

void push_front_action_test()
{
    push_front_test< std::deque<std::string> >();
}

