//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP
#define BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/permissions.hpp>
#if !defined(BOOST_INTERPROCESS_FORCE_GENERIC_EMULATION) && defined (BOOST_INTERPROCESS_WINDOWS)
   #include <boost/interprocess/sync/windows/named_recursive_mutex.hpp>
   #define BOOST_INTERPROCESS_USE_WINDOWS
#else
   #include <boost/interprocess/sync/shm/named_recursive_mutex.hpp>
#endif

//!\file
//!Describes a named named_recursive_mutex class for inter-process synchronization

namespace boost {
namespace interprocess {

/// @cond
namespace ipcdetail{ class interprocess_tester; }
/// @endcond

//!A recursive mutex with a global name, so it can be found from different
//!processes. This mutex can't be placed in shared memory, and
//!each process should have it's own named_recursive_mutex.
class named_recursive_mutex
{
   /// @cond
   //Non-copyable
   named_recursive_mutex();
   named_recursive_mutex(const named_recursive_mutex &);
   named_recursive_mutex &operator=(const named_recursive_mutex &);
   /// @endcond
   public:

   //!Creates a global recursive_mutex with a name.
   //!If the recursive_mutex can't be created throws interprocess_exception
   named_recursive_mutex(create_only_t create_only, const char *name, const permissions &perm = permissions());

   //!Opens or creates a global recursive_mutex with a name.
   //!If the recursive_mutex is created, this call is equivalent to
   //!named_recursive_mutex(create_only_t, ... )
   //!If the recursive_mutex is already created, this call is equivalent
   //!named_recursive_mutex(open_only_t, ... )
   //!Does not throw
   named_recursive_mutex(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   //!Opens a global recursive_mutex with a name if that recursive_mutex is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_recursive_mutex(open_only_t open_only, const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_recursive_mutex();

   //!Unlocks a previously locked
   //!named_recursive_mutex.
   void unlock();

   //!Locks named_recursive_mutex, sleeps when named_recursive_mutex is already locked.
   //!Throws interprocess_exception if a severe error is found.
   void lock();

   //!Tries to lock the named_recursive_mutex, returns false when named_recursive_mutex
   //!is already locked, returns true when success.
   //!Throws interprocess_exception if a severe error is found.
   bool try_lock();

   //!Tries to lock the named_recursive_mutex until time abs_time,
   //!Returns false when timeout expires, returns true when locks.
   //!Throws interprocess_exception if a severe error is found
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   //!Erases a named recursive mutex
   //!from the system
   static bool remove(const char *name);

   /// @cond
   private:
   friend class ipcdetail::interprocess_tester;
   void dont_close_on_destruction();

   #if defined(BOOST_INTERPROCESS_USE_WINDOWS)
      typedef ipcdetail::windows_named_recursive_mutex   impl_t;
      #undef BOOST_INTERPROCESS_USE_WINDOWS
   #else
      typedef ipcdetail::shm_named_recursive_mutex impl_t;
   #endif
   impl_t m_mut;

   /// @endcond
};

/// @cond

inline named_recursive_mutex::~named_recursive_mutex()
{}

inline void named_recursive_mutex::dont_close_on_destruction()
{  ipcdetail::interprocess_tester::dont_close_on_destruction(m_mut);  }

inline named_recursive_mutex::named_recursive_mutex(create_only_t, const char *name, const permissions &perm)
   :  m_mut  (create_only, name, perm)
{}

inline named_recursive_mutex::named_recursive_mutex(open_or_create_t, const char *name, const permissions &perm)
   :  m_mut  (open_or_create, name, perm)
{}

inline named_recursive_mutex::named_recursive_mutex(open_only_t, const char *name)
   :  m_mut   (open_only, name)
{}

inline void named_recursive_mutex::lock()
{  m_mut.lock();  }

inline void named_recursive_mutex::unlock()
{  m_mut.unlock();  }

inline bool named_recursive_mutex::try_lock()
{  return m_mut.try_lock();  }

inline bool named_recursive_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   return m_mut.timed_lock(abs_time);
}

inline bool named_recursive_mutex::remove(const char *name)
{  return impl_t::remove(name); }

/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_RECURSIVE_MUTEX_HPP
