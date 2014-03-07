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
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "named_creation_template.hpp"
#include "mutex_test_template.hpp"
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

static const std::size_t SemCount      = 1;
static const std::size_t RecSemCount   = 100;
static const char *      SemName = test::get_process_id_name();

//This wrapper is necessary to plug this class
//in lock tests
class lock_test_wrapper
   : public named_semaphore
{
   public:

   lock_test_wrapper(create_only_t, const char *name, unsigned int count = 1)
      :  named_semaphore(create_only, name, count)
   {}

   lock_test_wrapper(open_only_t, const char *name)
      :  named_semaphore(open_only, name)
   {}

   lock_test_wrapper(open_or_create_t, const char *name, unsigned int count = 1)
      :  named_semaphore(open_or_create, name, count)
   {}

   ~lock_test_wrapper()
   {}

   void lock()
   {  this->wait();  }

   bool try_lock()
   {  return this->try_wait();  }

   bool timed_lock(const boost::posix_time::ptime &pt)
   {  return this->timed_wait(pt);  }

   void unlock()
   {  this->post();  }
};

//This wrapper is necessary to plug this class
//in recursive tests
class recursive_test_wrapper
   :  public lock_test_wrapper
{
   public:
   recursive_test_wrapper(create_only_t, const char *name)
      :  lock_test_wrapper(create_only, name, RecSemCount)
   {}

   recursive_test_wrapper(open_only_t, const char *name)
      :  lock_test_wrapper(open_only, name)
   {}

   recursive_test_wrapper(open_or_create_t, const char *name)
      :  lock_test_wrapper(open_or_create, name, RecSemCount)
   {}
};

bool test_named_semaphore_specific()
{
   //Test persistance
   {
      named_semaphore sem(create_only, SemName, 3);
   }
   {
      named_semaphore sem(open_only, SemName);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == false);
      sem.post();
   }
   {
      named_semaphore sem(open_only, SemName);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == true);
      BOOST_INTERPROCESS_CHECK(sem.try_wait() == false);
   }

   named_semaphore::remove(SemName);
   return true;
}

int main ()
{
   try{
      named_semaphore::remove(SemName);
      test::test_named_creation< test::named_sync_creation_test_wrapper<lock_test_wrapper> >();
      test::test_all_lock< test::named_sync_wrapper<lock_test_wrapper> >();
      test::test_all_mutex<test::named_sync_wrapper<lock_test_wrapper> >();
      test::test_all_recursive_lock<test::named_sync_wrapper<recursive_test_wrapper> >();
      test_named_semaphore_specific();
   }
   catch(std::exception &ex){
      named_semaphore::remove(SemName);
      std::cout << ex.what() << std::endl;
      return 1;
   }
   named_semaphore::remove(SemName);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
