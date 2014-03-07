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
#include "named_creation_template.hpp"
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

struct mutex_deleter
{
   ~mutex_deleter()
   {  named_upgradable_mutex::remove(test::get_process_id_name()); }
};

//This wrapper is necessary to have a default constructor
//in generic mutex_test_template functions
class named_upgradable_mutex_lock_test_wrapper
   : public named_upgradable_mutex
{
   public:
   named_upgradable_mutex_lock_test_wrapper()
      :  named_upgradable_mutex(open_or_create, test::get_process_id_name())
   {  ++count_;   }

   ~named_upgradable_mutex_lock_test_wrapper()
   {
      if(--count_){
         ipcdetail::interprocess_tester::
            dont_close_on_destruction(static_cast<named_upgradable_mutex&>(*this));
      }
   }

   static int count_;
};

int named_upgradable_mutex_lock_test_wrapper::count_ = 0;


//This wrapper is necessary to have a common constructor
//in generic named_creation_template functions
class named_upgradable_mutex_creation_test_wrapper
   : public mutex_deleter, public named_upgradable_mutex
{
   public:
   named_upgradable_mutex_creation_test_wrapper
      (create_only_t)
      :  named_upgradable_mutex(create_only, test::get_process_id_name())
   {  ++count_;   }

   named_upgradable_mutex_creation_test_wrapper
      (open_only_t)
      :  named_upgradable_mutex(open_only, test::get_process_id_name())
   {  ++count_;   }

   named_upgradable_mutex_creation_test_wrapper
      (open_or_create_t)
      :  named_upgradable_mutex(open_or_create, test::get_process_id_name())
   {  ++count_;   }

   ~named_upgradable_mutex_creation_test_wrapper()
   {
      if(--count_){
         ipcdetail::interprocess_tester::
            dont_close_on_destruction(static_cast<named_upgradable_mutex&>(*this));
      }
   }

   static int count_;
};

int named_upgradable_mutex_creation_test_wrapper::count_ = 0;

int main ()
{
   try{
      named_upgradable_mutex::remove(test::get_process_id_name());
      test::test_named_creation< test::named_sync_creation_test_wrapper<named_upgradable_mutex> >();
      test::test_all_lock< test::named_sync_wrapper<named_upgradable_mutex> >();
      test::test_all_mutex<test::named_sync_wrapper<named_upgradable_mutex> >();
      test::test_all_sharable_mutex<test::named_sync_wrapper<named_upgradable_mutex> >();
   }
   catch(std::exception &ex){
      named_upgradable_mutex::remove(test::get_process_id_name());
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_upgradable_mutex::remove(test::get_process_id_name());
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
