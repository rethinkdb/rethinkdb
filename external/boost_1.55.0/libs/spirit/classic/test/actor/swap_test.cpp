/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

///////////////////////////////////////////////////////////////////////////////
// Test suite for and_assign_actor
///////////////////////////////////////////////////////////////////////////////

#include "action_tests.hpp"
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_swap_actor.hpp>

void swap_action_test()
{
    using namespace BOOST_SPIRIT_CLASSIC_NS;

    const char* cp = "63";
    const char* cp_first = cp;
    const char* cp_last = cp + test_impl::string_length(cp);
    std::vector<int> v1,v2;

    v1.push_back(0);
    v1.push_back(1);

    v2.push_back(2);
    v2.push_back(3);

    scanner<char const*> scan( cp_first, cp_last );
    match<> hit;

    hit = int_p[ swap_a(v1,v2)].parse(scan);
    BOOST_CHECK(hit);
    BOOST_CHECK_EQUAL(scan.first, scan.last);
    BOOST_CHECK(v1.size()==2);
    BOOST_CHECK(v2.size()==2);
    BOOST_CHECK_EQUAL(v2[0],0);
    BOOST_CHECK_EQUAL(v2[1],1);
    BOOST_CHECK_EQUAL(v1[0],2);
    BOOST_CHECK_EQUAL(v1[1],3);
}


