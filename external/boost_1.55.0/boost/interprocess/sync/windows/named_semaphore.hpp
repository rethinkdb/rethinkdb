 //////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WINDOWS_NAMED_SEMAPHORE_HPP
#define BOOST_INTERPROCESS_WINDOWS_NAMED_SEMAPHORE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/sync/windows/named_sync.hpp>
#include <boost/interprocess/sync/windows/winapi_semaphore_wrapper.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {



class windows_named_semaphore
{
   /// @cond

   //Non-copyable
   windows_named_semaphore();
   windows_named_semaphore(const windows_named_semaphore &);
   windows_named_semaphore &operator=(const windows_named_semaphore &);
   /// @endcond

   public:
   windows_named_semaphore(create_only_t, const char *name, unsigned int initialCount, const permissions &perm = permissions());

   windows_named_semaphore(open_or_create_t, const char *name, unsigned int initialCount, const permissions &perm = permissions());

   windows_named_semaphore(open_only_t, const char *name);

   ~windows_named_semaphore();

   void post();
   void wait();
   bool try_wait();
   bool timed_wait(const boost::posix_time::ptime &abs_time);

   static bool remove(const char *name);

   /// @cond
   private:
   friend class interprocess_tester;
   void dont_close_on_destruction();
   winapi_semaphore_wrapper m_sem_wrapper;
   windows_named_sync m_named_sync;

   class named_sem_callbacks : public windows_named_sync_interface
   {
      public:
      typedef __int64 sem_count_t;
      named_sem_callbacks(winapi_semaphore_wrapper &sem_wrapper, sem_count_t sem_cnt)
         : m_sem_wrapper(sem_wrapper), m_sem_count(sem_cnt)
      {}

      virtual std::size_t get_data_size() const
      {  return sizeof(sem_count_t);   }

      virtual const void *buffer_with_final_data_to_file()
      {  return &m_sem_count; }

      virtual const void *buffer_with_init_data_to_file()
      {  return &m_sem_count; }

      virtual void *buffer_to_store_init_data_from_file()
      {  return &m_sem_count; }

      virtual bool open(create_enum_t, const char *id_name)
      {
         std::string aux_str  = "Global\\bipc.sem.";
         aux_str += id_name;
         //
         permissions sem_perm;
         sem_perm.set_unrestricted();
         bool created;
         return m_sem_wrapper.open_or_create
            ( aux_str.c_str(), static_cast<long>(m_sem_count)
            , winapi_semaphore_wrapper::MaxCount, sem_perm, created);
      }

      virtual void close()
      {
         m_sem_wrapper.close();
      }

      virtual ~named_sem_callbacks()
      {}

      private:
      sem_count_t m_sem_count;
      winapi_semaphore_wrapper&     m_sem_wrapper;
   };

   /// @endcond
};

inline windows_named_semaphore::~windows_named_semaphore()
{
   named_sem_callbacks callbacks(m_sem_wrapper, m_sem_wrapper.value());
   m_named_sync.close(callbacks);
}

inline void windows_named_semaphore::dont_close_on_destruction()
{}

inline windows_named_semaphore::windows_named_semaphore
   (create_only_t, const char *name, unsigned int initial_count, const permissions &perm)
   : m_sem_wrapper()
{
   named_sem_callbacks callbacks(m_sem_wrapper, initial_count);
   m_named_sync.open_or_create(DoCreate, name, perm, callbacks);
}

inline windows_named_semaphore::windows_named_semaphore
   (open_or_create_t, const char *name, unsigned int initial_count, const permissions &perm)
   : m_sem_wrapper()
{
   named_sem_callbacks callbacks(m_sem_wrapper, initial_count);
   m_named_sync.open_or_create(DoOpenOrCreate, name, perm, callbacks);
}

inline windows_named_semaphore::windows_named_semaphore(open_only_t, const char *name)
   : m_sem_wrapper()
{
   named_sem_callbacks callbacks(m_sem_wrapper, 0);
   m_named_sync.open_or_create(DoOpen, name, permissions(), callbacks);
}

inline void windows_named_semaphore::post()
{
   m_sem_wrapper.post();
}

inline void windows_named_semaphore::wait()
{
   m_sem_wrapper.wait();
}

inline bool windows_named_semaphore::try_wait()
{
   return m_sem_wrapper.try_wait();
}

inline bool windows_named_semaphore::timed_wait(const boost::posix_time::ptime &abs_time)
{
   return m_sem_wrapper.timed_wait(abs_time);
}

inline bool windows_named_semaphore::remove(const char *name)
{
   return windows_named_sync::remove(name);
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_WINDOWS_NAMED_SEMAPHORE_HPP
