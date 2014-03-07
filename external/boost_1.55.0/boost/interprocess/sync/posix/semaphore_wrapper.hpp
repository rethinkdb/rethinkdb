//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_POSIX_SEMAPHORE_WRAPPER_HPP
#define BOOST_INTERPROCESS_POSIX_SEMAPHORE_WRAPPER_HPP

#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/tmp_dir_helpers.hpp>
#include <boost/interprocess/permissions.hpp>

#include <fcntl.h>      //O_CREAT, O_*...
#include <unistd.h>     //close
#include <string>       //std::string
#include <semaphore.h>  //sem_* family, SEM_VALUE_MAX
#include <sys/stat.h>   //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#include <boost/assert.hpp>

#ifdef SEM_FAILED
#define BOOST_INTERPROCESS_POSIX_SEM_FAILED (reinterpret_cast<sem_t*>(SEM_FAILED))
#else
#define BOOST_INTERPROCESS_POSIX_SEM_FAILED (reinterpret_cast<sem_t*>(-1))
#endif

#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS
#include <boost/interprocess/sync/posix/ptime_to_timespec.hpp>
#else
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/spin/wait.hpp>
#endif

namespace boost {
namespace interprocess {
namespace ipcdetail {

inline bool semaphore_open
   (sem_t *&handle, create_enum_t type, const char *origname,
    unsigned int count = 0, const permissions &perm = permissions())
{
   std::string name;
   #ifndef BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SEMAPHORES
   add_leading_slash(origname, name);
   #else
   create_tmp_and_clean_old_and_get_filename(origname, name);
   #endif

   //Create new mapping
   int oflag = 0;
   switch(type){
      case DoOpen:
      {
         //No addition
         handle = ::sem_open(name.c_str(), oflag);
      }
      break;
      case DoOpenOrCreate:
      case DoCreate:
      {
         while(1){
            oflag = (O_CREAT | O_EXCL);
            handle = ::sem_open(name.c_str(), oflag, perm.get_permissions(), count);
            if(handle != BOOST_INTERPROCESS_POSIX_SEM_FAILED){
               //We can't change semaphore permissions!
               //::fchmod(handle, perm.get_permissions());
               break;
            }
            else if(errno == EEXIST && type == DoOpenOrCreate){
               oflag = 0;
               if( (handle = ::sem_open(name.c_str(), oflag)) != BOOST_INTERPROCESS_POSIX_SEM_FAILED
                   || (errno != ENOENT) ){
                  break;
               }
            }
            else{
               break;
            }
         }
      }
      break;
      default:
      {
         error_info err(other_error);
         throw interprocess_exception(err);
      }
   }

   //Check for error
   if(handle == BOOST_INTERPROCESS_POSIX_SEM_FAILED){
      throw interprocess_exception(error_info(errno));
   }

   return true;
}

inline void semaphore_close(sem_t *handle)
{
   int ret = sem_close(handle);
   if(ret != 0){
      BOOST_ASSERT(0);
   }
}

inline bool semaphore_unlink(const char *semname)
{
   try{
      std::string sem_str;
      #ifndef BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SEMAPHORES
      add_leading_slash(semname, sem_str);
      #else
      tmp_filename(semname, sem_str);
      #endif
      return 0 == sem_unlink(sem_str.c_str());
   }
   catch(...){
      return false;
   }
}

inline void semaphore_init(sem_t *handle, unsigned int initialCount)
{
   int ret = sem_init(handle, 1, initialCount);
   //According to SUSV3 version 2003 edition, the return value of a successful
   //sem_init call is not defined, but -1 is returned on failure.
   //In the future, a successful call might be required to return 0.
   if(ret == -1){
      throw interprocess_exception(system_error_code());
   }
}

inline void semaphore_destroy(sem_t *handle)
{
   int ret = sem_destroy(handle);
   if(ret != 0){
      BOOST_ASSERT(0);
   }
}

inline void semaphore_post(sem_t *handle)
{
   int ret = sem_post(handle);
   if(ret != 0){
      throw interprocess_exception(system_error_code());
   }
}

inline void semaphore_wait(sem_t *handle)
{
   int ret = sem_wait(handle);
   if(ret != 0){
      throw interprocess_exception(system_error_code());
   }
}

inline bool semaphore_try_wait(sem_t *handle)
{
   int res = sem_trywait(handle);
   if(res == 0)
      return true;
   if(system_error_code() == EAGAIN){
      return false;
   }
   throw interprocess_exception(system_error_code());
   return false;
}

inline bool semaphore_timed_wait(sem_t *handle, const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      semaphore_wait(handle);
      return true;
   }
   #ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS
   timespec tspec = ptime_to_timespec(abs_time);
   for (;;){
      int res = sem_timedwait(handle, &tspec);
      if(res == 0)
         return true;
      if (res > 0){
         //buggy glibc, copy the returned error code to errno
         errno = res;
      }
      if(system_error_code() == ETIMEDOUT){
         return false;
      }
      throw interprocess_exception(system_error_code());
   }
   return false;
   #else //#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS
   boost::posix_time::ptime now;
   spin_wait swait;
   do{
      if(semaphore_try_wait(handle))
         return true;
      swait.yield();
   }while((now = microsec_clock::universal_time()) < abs_time);
   return false;
   #endif   //#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#endif   //#ifndef BOOST_INTERPROCESS_POSIX_SEMAPHORE_WRAPPER_HPP
