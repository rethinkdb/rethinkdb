//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/result.hpp>

#include <boost/static_assert.hpp>



namespace sc = boost::statechart;



int main()
{
  // sc::result has no default ctor
  sc::result r;

  #ifdef NDEBUG
    // Test only fails in DEBUG mode. Forcing failure...
    BOOST_STATIC_ASSERT( false );
  #endif

  return 0;
}
