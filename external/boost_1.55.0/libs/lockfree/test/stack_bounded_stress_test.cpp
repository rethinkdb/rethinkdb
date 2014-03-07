//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/lockfree/stack.hpp>

#define BOOST_TEST_MAIN
#ifdef BOOST_LOCKFREE_INCLUDE_TESTS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

#include "test_common.hpp"

BOOST_AUTO_TEST_CASE( stack_test_bounded )
{
    typedef queue_stress_tester<true> tester_type;
    boost::scoped_ptr<tester_type> tester(new tester_type(4, 4) );

    boost::lockfree::stack<long> q(128);
    tester->run(q);
}
