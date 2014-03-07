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
#include <boost/interprocess/sync/named_recursive_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "mutex_test_template.hpp"
#include "named_creation_template.hpp"
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

int main ()
{
   try{
      named_recursive_mutex::remove(test::get_process_id_name());
      test::test_named_creation< test::named_sync_creation_test_wrapper<named_recursive_mutex> >();
      test::test_all_lock< test::named_sync_wrapper<named_recursive_mutex> >();
      test::test_all_mutex<test::named_sync_wrapper<named_recursive_mutex> >();
      test::test_all_recursive_lock<test::named_sync_wrapper<named_recursive_mutex> >();
   }
   catch(std::exception &ex){
      named_recursive_mutex::remove(test::get_process_id_name());
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_recursive_mutex::remove(test::get_process_id_name());
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>

