//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <libs/statechart/test/ThrowingBoostAssert.hpp>
#include <boost/statechart/result.hpp>

#include <boost/test/test_tools.hpp>

#include <stdexcept> // std::logic_error



namespace sc = boost::statechart;



void make_unconsumed_result()
{
  // We cannot test sc::result in its natural environment here because a
  // failing assert triggers a stack unwind, what will lead to another
  // failing assert...

  // Creates a temp sc::result value which is destroyed immediately
  sc::detail::result_utility::make_result( sc::detail::do_discard_event );
}

// TODO: The current test setup lets an exception propagate out of a
// destructor. Find a better way to test this.
int test_main( int, char* [] )
{
  #ifdef NDEBUG
    BOOST_REQUIRE_NO_THROW( make_unconsumed_result() );
  #else
    BOOST_REQUIRE_THROW( make_unconsumed_result(), std::logic_error );
  #endif

  return 0;
}
