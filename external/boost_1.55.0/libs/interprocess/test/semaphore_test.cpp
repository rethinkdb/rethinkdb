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
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "named_creation_template.hpp"
#include "mutex_test_template.hpp"

static const std::size_t SemCount      = 1;
static const std::size_t RecSemCount   = 100;

//This wrapper is necessary to plug this class
//in named creation tests and interprocess_mutex tests
class semaphore_test_wrapper
   : public boost::interprocess::interprocess_semaphore
{
   public:
   semaphore_test_wrapper()
      :  boost::interprocess::interprocess_semaphore(SemCount)
   {}

   void lock()
   {  this->wait();  }

   bool try_lock()
   {  return this->try_wait();  }

   bool timed_lock(const boost::posix_time::ptime &pt)
   {  return this->timed_wait(pt);  }

   void unlock()
   {  this->post();  }

   protected:
   semaphore_test_wrapper(int initial_count)
      :  boost::interprocess::interprocess_semaphore(initial_count)
   {}
};

//This wrapper is necessary to plug this class
//in recursive tests
class recursive_semaphore_test_wrapper
   :  public semaphore_test_wrapper
{
   public:
   recursive_semaphore_test_wrapper()
      :  semaphore_test_wrapper(RecSemCount)
   {}
};

int main ()
{
   using namespace boost::interprocess;

   test::test_all_lock<semaphore_test_wrapper>();
   test::test_all_recursive_lock<recursive_semaphore_test_wrapper>();
   test::test_all_mutex<semaphore_test_wrapper>();
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
