//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/sync/interprocess_condition_any.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/spin/mutex.hpp>

#include "condition_test_template.hpp"

using namespace boost::interprocess;

int main ()
{
   if(!test::do_test_condition<interprocess_condition_any, interprocess_mutex>())
      return 1;
   if(!test::do_test_condition<interprocess_condition_any, ipcdetail::spin_mutex>())
      return 1;
   if(!test::do_test_condition<interprocess_condition_any, interprocess_recursive_mutex>())
      return 1;

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
