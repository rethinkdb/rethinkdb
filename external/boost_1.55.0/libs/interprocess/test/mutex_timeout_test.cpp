//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

// enable timeout feature
#define BOOST_INTERPROCESS_ENABLE_TIMEOUT_WHEN_LOCKING
#define BOOST_INTERPROCESS_TIMEOUT_WHEN_LOCKING_DURATION_MS 1000

#include <boost/assert.hpp>
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include "mutex_test_template.hpp"

int main ()
{
   using namespace boost::interprocess;
   test::test_mutex_lock_timeout<interprocess_mutex>();
   test::test_mutex_lock_timeout<interprocess_recursive_mutex>();

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
