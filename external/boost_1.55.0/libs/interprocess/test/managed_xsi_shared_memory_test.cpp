//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#if defined(BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS)

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_xsi_shared_memory.hpp>
#include <boost/interprocess/detail/file_wrapper.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <cstdio>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

void remove_shared_memory(const xsi_key &key)
{
   try{
      xsi_shared_memory xsi(open_only, key);
      xsi_shared_memory::remove(xsi.get_shmid());
   }
   catch(interprocess_exception &e){
      if(e.get_error_code() != not_found_error)
         throw;
   }
}

class xsi_shared_memory_remover
{
   public:
   xsi_shared_memory_remover(xsi_shared_memory &xsi_shm)
      : xsi_shm_(xsi_shm)
   {}

   ~xsi_shared_memory_remover()
   {  xsi_shared_memory::remove(xsi_shm_.get_shmid());  }
   private:
   xsi_shared_memory & xsi_shm_;
};

inline std::string get_filename()
{
   std::string ret (ipcdetail::get_temporary_path());
   ret += "/";
   ret += test::get_process_id_name();
   return ret;
}

int main ()
{
   const int ShmemSize          = 65536;
   std::string filename = get_filename();
   const char *const ShmemName = filename.c_str();

   file_mapping::remove(ShmemName);
   {  ipcdetail::file_wrapper(create_only, ShmemName, read_write); }
   xsi_key key(ShmemName, 1);
   file_mapping::remove(ShmemName);
   int shmid;

   //STL compatible allocator object for memory-mapped shmem
   typedef allocator<int, managed_xsi_shared_memory::segment_manager>
      allocator_int_t;
   //A vector that uses that allocator
   typedef boost::interprocess::vector<int, allocator_int_t> MyVect;

   {
      //Remove the shmem it is already created
      remove_shared_memory(key);

      const int max              = 100;
      void *array[max];
      //Named allocate capable shared memory allocator
      managed_xsi_shared_memory shmem(create_only, key, ShmemSize);
      shmid = shmem.get_shmid();
      int i;
      //Let's allocate some memory
      for(i = 0; i < max; ++i){
         array[i] = shmem.allocate(i+1);
      }

      //Deallocate allocated memory
      for(i = 0; i < max; ++i){
         shmem.deallocate(array[i]);
      }
   }

   {
      //Remove the shmem it is already created
      xsi_shared_memory::remove(shmid);

      //Named allocate capable memory mapped shmem managed memory class
      managed_xsi_shared_memory shmem(create_only, key, ShmemSize);
      shmid = shmem.get_shmid();

      //Construct the STL-like allocator with the segment manager
      const allocator_int_t myallocator (shmem.get_segment_manager());

      //Construct vector
      MyVect *shmem_vect = shmem.construct<MyVect> ("MyVector") (myallocator);

      //Test that vector can be found via name
      if(shmem_vect != shmem.find<MyVect>("MyVector").first)
         return -1;

      //Destroy and check it is not present
      shmem.destroy<MyVect> ("MyVector");
      if(0 != shmem.find<MyVect>("MyVector").first)
         return -1;

      //Construct a vector in the memory-mapped shmem
      shmem_vect = shmem.construct<MyVect> ("MyVector") (myallocator);
   }
   {
      //Map preexisting shmem again in memory
      managed_xsi_shared_memory shmem(open_only, key);
      shmid = shmem.get_shmid();

      //Check vector is still there
      MyVect *shmem_vect = shmem.find<MyVect>("MyVector").first;
      if(!shmem_vect)
         return -1;
   }


   {
      //Now destroy the vector
      {
         //Map preexisting shmem again in memory
         managed_xsi_shared_memory shmem(open_only, key);
         shmid = shmem.get_shmid();

         //Destroy and check it is not present
         shmem.destroy<MyVect>("MyVector");
         if(0 != shmem.find<MyVect>("MyVector").first)
            return -1;
      }

      {
         //Now test move semantics
         managed_xsi_shared_memory original(open_only, key);
         managed_xsi_shared_memory move_ctor(boost::move(original));
         managed_xsi_shared_memory move_assign;
         move_assign = boost::move(move_ctor);
         move_assign.swap(original);
      }
   }

   xsi_shared_memory::remove(shmid);
   return 0;
}

#else

int main()
{
   return 0;
}

#endif  //#ifndef BOOST_INTERPROCESS_XSI_SHARED_MEMORY_OBJECTS

#include <boost/interprocess/detail/config_end.hpp>
