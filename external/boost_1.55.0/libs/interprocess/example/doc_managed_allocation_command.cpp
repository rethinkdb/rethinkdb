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
//[doc_managed_allocation_command
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>
//<-
#include "../test/get_process_id_name.hpp"
//->

int main()
{
   using namespace boost::interprocess;

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

   //Managed memory segment that allocates portions of a shared memory
   //segment with the default management algorithm
   //<-
   #if 1
   managed_shared_memory managed_shm(create_only, test::get_process_id_name(), 10000*sizeof(std::size_t));
   #else
   //->
   managed_shared_memory managed_shm(create_only, "MySharedMemory", 10000*sizeof(std::size_t));
   //<-
   #endif
   //->

   //Allocate at least 100 bytes, 1000 bytes if possible
   managed_shared_memory::size_type min_size = 100, preferred_size = 1000;
   managed_shared_memory::size_type received_size;
   std::size_t *ptr = managed_shm.allocation_command<std::size_t>
      (boost::interprocess::allocate_new, min_size, preferred_size, received_size).first;

   //Received size must be bigger than min_size
   assert(received_size >= min_size);

   //Get free memory
   managed_shared_memory::size_type free_memory_after_allocation = managed_shm.get_free_memory();

   //Now write the data
   for(std::size_t i = 0; i < received_size; ++i) ptr[i] = i;

   //Now try to triplicate the buffer. We won't admit an expansion
   //lower to the double of the original buffer.
   //This "should" be successful since no other class is allocating
   //memory from the segment
   managed_shared_memory::size_type expanded_size;
   std::pair<std::size_t *, bool> ret = managed_shm.allocation_command
      (boost::interprocess::expand_fwd, received_size*2, received_size*3, expanded_size, ptr);

   //Check invariants
   assert(ret.second == true);
   assert(ret.first == ptr);
   assert(expanded_size >= received_size*2);

   //Get free memory and compare
   managed_shared_memory::size_type free_memory_after_expansion = managed_shm.get_free_memory();
   assert(free_memory_after_expansion < free_memory_after_allocation);

   //Write new values
   for(std::size_t i = received_size; i < expanded_size; ++i)  ptr[i] = i;

   //Try to shrink approximately to min_size, but the new size
   //should be smaller than min_size*2.
   //This "should" be successful since no other class is allocating
   //memory from the segment
   managed_shared_memory::size_type shrunk_size;
   ret = managed_shm.allocation_command
      (boost::interprocess::shrink_in_place, min_size*2, min_size, shrunk_size, ptr);

   //Check invariants
   assert(ret.second == true);
   assert(ret.first == ptr);
   assert(shrunk_size <= min_size*2);
   assert(shrunk_size >= min_size);

   //Get free memory and compare
   managed_shared_memory::size_type free_memory_after_shrinking = managed_shm.get_free_memory();
   assert(free_memory_after_shrinking > free_memory_after_expansion);

   //Deallocate the buffer
   managed_shm.deallocate(ptr);
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
