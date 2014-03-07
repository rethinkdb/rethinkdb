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
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdio>
#include <string>
#include "get_process_id_name.hpp"

using namespace boost::interprocess;

int main ()
{
   const int ShmemSize          = 65536;
   const char *const ShmemName = test::get_process_id_name();

   //STL compatible allocator object for memory-mapped shmem
   typedef allocator<int, managed_shared_memory::segment_manager>
      allocator_int_t;
   //A vector that uses that allocator
   typedef boost::interprocess::vector<int, allocator_int_t> MyVect;

   {
      //Remove the shmem it is already created
      shared_memory_object::remove(ShmemName);

      const int max              = 100;
      void *array[max];
      //Named allocate capable shared memory allocator
      managed_shared_memory shmem(create_only, ShmemName, ShmemSize);

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
      shared_memory_object::remove(ShmemName);

      //Named allocate capable memory mapped shmem managed memory class
      managed_shared_memory shmem(create_only, ShmemName, ShmemSize);

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
      managed_shared_memory shmem(open_only, ShmemName);

      //Check vector is still there
      MyVect *shmem_vect = shmem.find<MyVect>("MyVector").first;
      if(!shmem_vect)
         return -1;
   }
   {
      {
         //Map preexisting shmem again in copy-on-write
         managed_shared_memory shmem(open_copy_on_write, ShmemName);

         //Check vector is still there
         MyVect *shmem_vect = shmem.find<MyVect>("MyVector").first;
         if(!shmem_vect)
            return -1;

         //Erase vector
         shmem.destroy_ptr(shmem_vect);

         //Make sure vector is erased
         shmem_vect = shmem.find<MyVect>("MyVector").first;
         if(shmem_vect)
            return -1;
      }
      //Now check vector is still in the shmem
      {
         //Map preexisting shmem again in copy-on-write
         managed_shared_memory shmem(open_copy_on_write, ShmemName);

         //Check vector is still there
         MyVect *shmem_vect = shmem.find<MyVect>("MyVector").first;
         if(!shmem_vect)
            return -1;
      }
   }
   {
      //Map preexisting shmem again in copy-on-write
      managed_shared_memory shmem(open_read_only, ShmemName);

      //Check vector is still there
      MyVect *shmem_vect = shmem.find<MyVect>("MyVector").first;
      if(!shmem_vect)
         return -1;
   }
   #ifndef BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS_NO_GROW
   {
      managed_shared_memory::size_type old_free_memory;
      {
         //Map preexisting shmem again in memory
         managed_shared_memory shmem(open_only, ShmemName);
         old_free_memory = shmem.get_free_memory();
      }

      //Now grow the shmem
      managed_shared_memory::grow(ShmemName, ShmemSize);

      //Map preexisting shmem again in memory
      managed_shared_memory shmem(open_only, ShmemName);

      //Check vector is still there
      MyVect *shmem_vect = shmem.find<MyVect>("MyVector").first;
      if(!shmem_vect)
         return -1;

      if(shmem.get_size() != (ShmemSize*2))
         return -1;
      if(shmem.get_free_memory() <= old_free_memory)
         return -1;
   }
   {
      managed_shared_memory::size_type old_free_memory, next_free_memory,
                  old_shmem_size, next_shmem_size, final_shmem_size;
      {
         //Map preexisting shmem again in memory
         managed_shared_memory shmem(open_only, ShmemName);
         old_free_memory = shmem.get_free_memory();
         old_shmem_size   = shmem.get_size();
      }

      //Now shrink the shmem
      managed_shared_memory::shrink_to_fit(ShmemName);

      {
         //Map preexisting shmem again in memory
         managed_shared_memory shmem(open_only, ShmemName);
         next_shmem_size = shmem.get_size();

         //Check vector is still there
         MyVect *shmem_vect = shmem.find<MyVect>("MyVector").first;
         if(!shmem_vect)
            return -1;

         next_free_memory = shmem.get_free_memory();
         if(next_free_memory >= old_free_memory)
            return -1;
         if(old_shmem_size <= next_shmem_size)
            return -1;
      }

      //Now destroy the vector
      {
         //Map preexisting shmem again in memory
         managed_shared_memory shmem(open_only, ShmemName);

         //Destroy and check it is not present
         shmem.destroy<MyVect>("MyVector");
         if(0 != shmem.find<MyVect>("MyVector").first)
            return -1;
      }

      //Now shrink the shmem
      managed_shared_memory::shrink_to_fit(ShmemName);
      {
         //Map preexisting shmem again in memory
         managed_shared_memory shmem(open_only, ShmemName);
         final_shmem_size = shmem.get_size();
         if(next_shmem_size <= final_shmem_size)
            return -1;
      }
   }
   #endif //ifndef BOOST_INTERPROCESS_POSIX_SHARED_MEMORY_OBJECTS_NO_GROW

   {
      //Now test move semantics
      managed_shared_memory original(open_only, ShmemName);
      managed_shared_memory move_ctor(boost::move(original));
      managed_shared_memory move_assign;
      move_assign = boost::move(move_ctor);
      move_assign.swap(original);
   }

   shared_memory_object::remove(ShmemName);
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
