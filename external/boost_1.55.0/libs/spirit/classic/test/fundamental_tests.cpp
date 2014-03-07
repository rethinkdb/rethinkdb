/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  Fundamental meta sublayer tests
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_meta.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

typedef ref_value_actor<char, assign_action> assign_actor;

///////////////////////////////////////////////////////////////////////////////
//
//  node_count_tests
//
///////////////////////////////////////////////////////////////////////////////
void
node_count_tests()
{
// simple types
    typedef chlit<char> plain_t;
    typedef optional<chlit<char> > optional_t;
    typedef action<chlit<char>, assign_actor> action_t;
    typedef sequence<chlit<char>, anychar_parser> sequence_t;

    BOOST_STATIC_ASSERT(1 == node_count<plain_t>::value);
    BOOST_STATIC_ASSERT(2 == node_count<optional_t>::value);
    BOOST_STATIC_ASSERT(2 == node_count<action_t>::value);
    BOOST_STATIC_ASSERT(3 == node_count<sequence_t>::value);

// more elaborate types
    typedef sequence<sequence<plain_t, action_t>, plain_t> sequence2_t;
    typedef sequence<plain_t, sequence<action_t, plain_t> > sequence3_t;

    BOOST_STATIC_ASSERT(6 == node_count<sequence2_t>::value);
    BOOST_STATIC_ASSERT(6 == node_count<sequence3_t>::value);
}

///////////////////////////////////////////////////////////////////////////////
//
//  leaf_count_tests
//
///////////////////////////////////////////////////////////////////////////////
void
leaf_count_tests()
{
// simple types
    typedef chlit<char> plain_t;
    typedef optional<chlit<char> > optional_t;
    typedef action<chlit<char>, assign_actor> action_t;
    typedef sequence<chlit<char>, anychar_parser> sequence_t;

    BOOST_STATIC_ASSERT(1 == leaf_count<plain_t>::value);
    BOOST_STATIC_ASSERT(1 == leaf_count<optional_t>::value);
    BOOST_STATIC_ASSERT(1 == leaf_count<action_t>::value);
    BOOST_STATIC_ASSERT(2 == leaf_count<sequence_t>::value);

// more elaborate types
    typedef sequence<sequence<plain_t, action_t>, plain_t> sequence2_t;
    typedef sequence<plain_t, sequence<action_t, plain_t> > sequence3_t;

    BOOST_STATIC_ASSERT(3 == leaf_count<sequence2_t>::value);
    BOOST_STATIC_ASSERT(3 == leaf_count<sequence3_t>::value);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    node_count_tests();
    leaf_count_tests();

    return boost::report_errors();
}

