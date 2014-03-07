/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

///////////////////////////////////////////////////////////////////////////////
// Test suite for assign_actor
///////////////////////////////////////////////////////////////////////////////

#include "action_tests.hpp"
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>

void assign_test()
{
    using namespace BOOST_SPIRIT_CLASSIC_NS;

    const char* cp = "63";
    const char* cp_first = cp;
    const char* cp_last = cp + test_impl::string_length(cp);
    int h=127;
    int hm=h;

    scanner<char const*> scan( cp_first, cp_last );
    match<> hit;

    hit = int_p[ assign_a(hm)].parse(scan);
    BOOST_CHECK(hit);
    BOOST_CHECK_EQUAL(scan.first, scan.last);

    h=63;
    BOOST_CHECK_EQUAL( hm,h);
}

void assign_test_ref()
{
    using namespace BOOST_SPIRIT_CLASSIC_NS;
    

    const char* cp = "63";
    const char* cp_first = cp;
    const char* cp_last = cp + test_impl::string_length(cp);
    int h=127;
    int hm=63;

    scanner<char const*> scan( cp_first, cp_last );
    match<> hit;

    hit = int_p[ assign_a(h,hm)].parse(scan);
    BOOST_CHECK(hit);
    BOOST_CHECK_EQUAL(scan.first, scan.last);

    BOOST_CHECK_EQUAL( hm,h);
}

void assign_action_test()
{
    assign_test();
    assign_test_ref();
}

