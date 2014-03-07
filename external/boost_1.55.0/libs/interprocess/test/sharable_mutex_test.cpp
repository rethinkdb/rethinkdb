//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include "mutex_test_template.hpp"
#include "sharable_mutex_test_template.hpp"
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "util.hpp"

int main ()
{
   using namespace boost::interprocess;

   test::test_all_lock<interprocess_sharable_mutex>();
   test::test_all_mutex<interprocess_sharable_mutex>();
   test::test_all_sharable_mutex<interprocess_sharable_mutex>();

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
