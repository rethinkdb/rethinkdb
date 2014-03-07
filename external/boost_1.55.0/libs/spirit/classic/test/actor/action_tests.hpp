/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_TEST_HPP
#define BOOST_SPIRIT_ACTOR_TEST_HPP
#include <boost/detail/lightweight_test.hpp>
#include "../impl/string_length.hpp"

///////////////////////////////////////////////////////////////////////////////
// Test suite for actors
///////////////////////////////////////////////////////////////////////////////
void assign_action_test();
void assign_key_action_test();
void clear_action_test();
void decrement_action_test();
void erase_action_test();
void increment_action_test();
void insert_key_action_test();
void insert_at_action_test();
void push_back_action_test();
void push_front_action_test();
void swap_action_test();

#define BOOST_CHECK(t) BOOST_TEST((t));
#define BOOST_CHECK_EQUAL(a, b) BOOST_TEST((a == b));
#define BOOST_MESSAGE(m) std::cout << m << std::endl

#endif
