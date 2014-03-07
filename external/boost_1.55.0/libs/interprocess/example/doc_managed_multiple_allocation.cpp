//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
//[doc_managed_multiple_allocation
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/detail/move.hpp> //boost::move
#include <cassert>//assert
#include <cstring>//std::memset
#include <new>    //std::nothrow
#include <vector> //std::vector
//<-
#include "../test/get_process_id_name.hpp"
//->

int main()
{
   using namespace boost::interprocess;
   typedef managed_shared_memory::multiallocation_chain multiallocation_chain;

   //Remove shared memory on construction and destruction
   struct shm_remove
   {
   //<-
   #if 1
      shm_remove() { shared_memory_object::remove(test::get_process_id_name()); }
      ~shm_remove(){ shared_memory_object::remove(test::get_process_id_name()); }
   #else
   //->
      shm_remove() { shared_memory_object::remove("MySharedMemory"); }
      ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
   //<-
   #endif
   //->
   } remover;
   //<-
   (void)remover;
   //->

   //<-
   #if 1
   managed_shared_memory managed_shm(create_only,test::get_process_id_name(), 65536);
   #else
   //->
   managed_shared_memory managed_shm(create_only,"MySharedMemory", 65536);
   //<-
   #endif
   //->

   //Allocate 16 elements of 100 bytes in a single call. Non-throwing version.
   multiallocation_chain chain;
   managed_shm.allocate_many(std::nothrow, 100, 16, chain);

   //Check if the memory allocation was successful
   if(chain.empty()) return 1;

   //Allocated buffers
   std::vector<void*> allocated_buffers;

   //Initialize our data
   while(!chain.empty()){
      void *buf = chain.pop_front();
      allocated_buffers.push_back(buf);
      //The iterator must be incremented before overwriting memory
      //because otherwise, the iterator is invalidated.
      std::memset(buf, 0, 100);
   }

   //Now deallocate
   while(!allocated_buffers.empty()){
      managed_shm.deallocate(allocated_buffers.back());
      allocated_buffers.pop_back();
   }

   //Allocate 10 buffers of different sizes in a single call. Throwing version
   managed_shared_memory::size_type sizes[10];
   for(std::size_t i = 0; i < 10; ++i)
      sizes[i] = i*3;

   managed_shm.allocate_many(sizes, 10, 1, chain);
   managed_shm.deallocate_many(chain);
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
