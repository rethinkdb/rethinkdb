//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_POSIX_NAMED_MUTEX_HPP
#define BOOST_INTERPROCESS_POSIX_NAMED_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/permissions.hpp>

#include <boost/interprocess/sync/posix/named_semaphore.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

class named_condition;

class posix_named_mutex
{
   /// @cond

   posix_named_mutex();
   posix_named_mutex(const posix_named_mutex &);
   posix_named_mutex &operator=(const posix_named_mutex &);
   friend class named_condition;
   /// @endcond

   public:
   posix_named_mutex(create_only_t create_only, const char *name, const permissions &perm = permissions());

   posix_named_mutex(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   posix_named_mutex(open_only_t open_only, const char *name);

   ~posix_named_mutex();

   void unlock();
   void lock();
   bool try_lock();
   bool timed_lock(const boost::posix_time::ptime &abs_time);
   static bool remove(const char *name);

   /// @cond
   private:
   friend class interprocess_tester;
   void dont_close_on_destruction();

   posix_named_semaphore m_sem;
   /// @endcond
};

/// @cond

inline posix_named_mutex::posix_named_mutex(create_only_t, const char *name, const permissions &perm)
   :  m_sem(create_only, name, 1, perm)
{}

inline posix_named_mutex::posix_named_mutex(open_or_create_t, const char *name, const permissions &perm)
   :  m_sem(open_or_create, name, 1, perm)
{}

inline posix_named_mutex::posix_named_mutex(open_only_t, const char *name)
   :  m_sem(open_only, name)
{}

inline void posix_named_mutex::dont_close_on_destruction()
{  interprocess_tester::dont_close_on_destruction(m_sem);  }

inline posix_named_mutex::~posix_named_mutex()
{}

inline void posix_named_mutex::lock()
{  m_sem.wait();  }

inline void posix_named_mutex::unlock()
{  m_sem.post();  }

inline bool posix_named_mutex::try_lock()
{  return m_sem.try_wait();  }

inline bool posix_named_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   return m_sem.timed_wait(abs_time);
}

inline bool posix_named_mutex::remove(const char *name)
{  return posix_named_semaphore::remove(name);   }

/// @endcond

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_POSIX_NAMED_MUTEX_HPP
