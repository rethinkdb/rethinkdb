//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_NAMED_SHARABLE_MUTEX_HPP
#define BOOST_INTERPROCESS_NAMED_SHARABLE_MUTEX_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/sync/shm/named_creation_functor.hpp>
#include <boost/interprocess/permissions.hpp>

//!\file
//!Describes a named sharable mutex class for inter-process synchronization

namespace boost {
namespace interprocess {

/// @cond
namespace ipcdetail{ class interprocess_tester; }
/// @endcond

class named_condition;

//!A sharable mutex with a global name, so it can be found from different
//!processes. This mutex can't be placed in shared memory, and
//!each process should have it's own named sharable mutex.
class named_sharable_mutex
{
   /// @cond
   //Non-copyable
   named_sharable_mutex();
   named_sharable_mutex(const named_sharable_mutex &);
   named_sharable_mutex &operator=(const named_sharable_mutex &);
   /// @endcond
   public:

   //!Creates a global sharable mutex with a name.
   //!If the sharable mutex can't be created throws interprocess_exception
   named_sharable_mutex(create_only_t create_only, const char *name, const permissions &perm = permissions());

   //!Opens or creates a global sharable mutex with a name.
   //!If the sharable mutex is created, this call is equivalent to
   //!named_sharable_mutex(create_only_t, ...)
   //!If the sharable mutex is already created, this call is equivalent to
   //!named_sharable_mutex(open_only_t, ... ).
   named_sharable_mutex(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   //!Opens a global sharable mutex with a name if that sharable mutex
   //!is previously.
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   named_sharable_mutex(open_only_t open_only, const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~named_sharable_mutex();

   //Exclusive locking

   //!Effects: The calling thread tries to obtain exclusive ownership of the mutex,
   //!   and if another thread has exclusive or sharable ownership of
   //!   the mutex, it waits until it can obtain the ownership.
   //!Throws: interprocess_exception on error.
   void lock();

   //!Effects: The calling thread tries to acquire exclusive ownership of the mutex
   //!   without waiting. If no other thread has exclusive or sharable
   //!   ownership of the mutex this succeeds.
   //!Returns: If it can acquire exclusive ownership immediately returns true.
   //!   If it has to wait, returns false.
   //!Throws: interprocess_exception on error.
   bool try_lock();

   //!Effects: The calling thread tries to acquire exclusive ownership of the mutex
   //!   waiting if necessary until no other thread has exclusive, or sharable
   //!   ownership of the mutex or abs_time is reached.
   //!Returns: If acquires exclusive ownership, returns true. Otherwise returns false.
   //!Throws: interprocess_exception on error.
   bool timed_lock(const boost::posix_time::ptime &abs_time);

   //!Precondition: The thread must have exclusive ownership of the mutex.
   //!Effects: The calling thread releases the exclusive ownership of the mutex.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock();

   //Sharable locking

   //!Effects: The calling thread tries to obtain sharable ownership of the mutex,
   //!   and if another thread has exclusive ownership of the mutex,
   //!   waits until it can obtain the ownership.
   //!Throws: interprocess_exception on error.
   void lock_sharable();

   //!Effects: The calling thread tries to acquire sharable ownership of the mutex
   //!   without waiting. If no other thread has exclusive ownership
   //!   of the mutex this succeeds.
   //!Returns: If it can acquire sharable ownership immediately returns true. If it
   //!   has to wait, returns false.
   //!Throws: interprocess_exception on error.
   bool try_lock_sharable();

   //!Effects: The calling thread tries to acquire sharable ownership of the mutex
   //!   waiting if necessary until no other thread has exclusive
   //!   ownership of the mutex or abs_time is reached.
   //!Returns: If acquires sharable ownership, returns true. Otherwise returns false.
   //!Throws: interprocess_exception on error.
   bool timed_lock_sharable(const boost::posix_time::ptime &abs_time);

   //!Precondition: The thread must have sharable ownership of the mutex.
   //!Effects: The calling thread releases the sharable ownership of the mutex.
   //!Throws: An exception derived from interprocess_exception on error.
   void unlock_sharable();

   //!Erases a named sharable mutex from the system.
   //!Returns false on error. Never throws.
   static bool remove(const char *name);

   /// @cond
   private:
   friend class ipcdetail::interprocess_tester;
   void dont_close_on_destruction();

   interprocess_sharable_mutex *mutex() const
   {  return static_cast<interprocess_sharable_mutex*>(m_shmem.get_user_address()); }

   typedef ipcdetail::managed_open_or_create_impl<shared_memory_object, 0, true, false> open_create_impl_t;
   open_create_impl_t m_shmem;
   typedef ipcdetail::named_creation_functor<interprocess_sharable_mutex> construct_func_t;
   /// @endcond
};

/// @cond

inline named_sharable_mutex::~named_sharable_mutex()
{}

inline named_sharable_mutex::named_sharable_mutex
   (create_only_t, const char *name, const permissions &perm)
   :  m_shmem  (create_only
               ,name
               ,sizeof(interprocess_sharable_mutex) +
                  open_create_impl_t::ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(ipcdetail::DoCreate)
               ,perm)
{}

inline named_sharable_mutex::named_sharable_mutex
   (open_or_create_t, const char *name, const permissions &perm)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(interprocess_sharable_mutex) +
                  open_create_impl_t::ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(ipcdetail::DoOpenOrCreate)
               ,perm)
{}

inline named_sharable_mutex::named_sharable_mutex
   (open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(ipcdetail::DoOpen))
{}

inline void named_sharable_mutex::dont_close_on_destruction()
{  ipcdetail::interprocess_tester::dont_close_on_destruction(m_shmem);  }

inline void named_sharable_mutex::lock()
{  this->mutex()->lock();  }

inline void named_sharable_mutex::unlock()
{  this->mutex()->unlock();  }

inline bool named_sharable_mutex::try_lock()
{  return this->mutex()->try_lock();  }

inline bool named_sharable_mutex::timed_lock
   (const boost::posix_time::ptime &abs_time)
{  return this->mutex()->timed_lock(abs_time);  }

inline void named_sharable_mutex::lock_sharable()
{  this->mutex()->lock_sharable();  }

inline void named_sharable_mutex::unlock_sharable()
{  this->mutex()->unlock_sharable();  }

inline bool named_sharable_mutex::try_lock_sharable()
{  return this->mutex()->try_lock_sharable();  }

inline bool named_sharable_mutex::timed_lock_sharable
   (const boost::posix_time::ptime &abs_time)
{  return this->mutex()->timed_lock_sharable(abs_time);  }

inline bool named_sharable_mutex::remove(const char *name)
{  return shared_memory_object::remove(name); }

/// @endcond

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_NAMED_SHARABLE_MUTEX_HPP
