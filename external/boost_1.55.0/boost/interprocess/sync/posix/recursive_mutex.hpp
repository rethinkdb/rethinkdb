//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// Parts of the pthread code come from Boost Threads code:
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_POSIX_RECURSIVE_MUTEX_HPP
#define BOOST_INTERPROCESS_DETAIL_POSIX_RECURSIVE_MUTEX_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <pthread.h>
#include <errno.h>
#include <boost/interprocess/sync/posix/pthread_helpers.hpp>
#include <boost/interprocess/sync/posix/ptime_to_timespec.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/exceptions.hpp>
#ifndef BOOST_INTERPROCESS_POSIX_TIMEOUTS
#  include <boost/interprocess/detail/os_thread_functions.hpp>
#  include <boost/interprocess/sync/spin/wait.hpp>
#endif
#include <boost/assert.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class posix_recursive_mutex
{
   posix_recursive_mutex(const posix_recursive_mutex &);
   posix_recursive_mutex &operator=(const posix_recursive_mutex &);
   public:

   posix_recursive_mutex();
   ~posix_recursive_mutex();

   void lock();
   bool try_lock();
   bool timed_lock(const boost::posix_time::ptime &abs_time);
   void unlock();

   private:
   pthread_mutex_t   m_mut;
};

inline posix_recursive_mutex::posix_recursive_mutex()
{
   mutexattr_wrapper mut_attr(true);
   mutex_initializer mut(m_mut, mut_attr);
   mut.release();
}

inline posix_recursive_mutex::~posix_recursive_mutex()
{
   int res = pthread_mutex_destroy(&m_mut);
   BOOST_ASSERT(res == 0);(void)res;
}

inline void posix_recursive_mutex::lock()
{
   if (pthread_mutex_lock(&m_mut) != 0)
      throw lock_exception();
}

inline bool posix_recursive_mutex::try_lock()
{
   int res = pthread_mutex_trylock(&m_mut);
   if (!(res == 0 || res == EBUSY))
      throw lock_exception();
   return res == 0;
}

inline bool posix_recursive_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   #ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS

   timespec ts = ptime_to_timespec(abs_time);
   int res = pthread_mutex_timedlock(&m_mut, &ts);
   if (res != 0 && res != ETIMEDOUT)
      throw lock_exception();
   return res == 0;

   #else //BOOST_INTERPROCESS_POSIX_TIMEOUTS

   //Obtain current count and target time
   boost::posix_time::ptime now = microsec_clock::universal_time();
   spin_wait swait;
   do{
      if(this->try_lock()){
         break;
      }
      now = microsec_clock::universal_time();

      if(now >= abs_time){
         return false;
      }
      // relinquish current time slice
      swait.yield();
   }while (true);
   return true;

   #endif   //BOOST_INTERPROCESS_POSIX_TIMEOUTS
}

inline void posix_recursive_mutex::unlock()
{
   int res = 0;
   res = pthread_mutex_unlock(&m_mut);
   BOOST_ASSERT(res == 0); (void)res;
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_POSIX_RECURSIVE_MUTEX_HPP
