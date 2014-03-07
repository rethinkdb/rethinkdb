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
#include <boost/interprocess/detail/workaround.hpp>
//[doc_private_node_allocator
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/private_node_allocator.hpp>
#include <cassert>
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

int main ()
{
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
   }  remover;
   //<-
   (void)remover;
   //->

   //Create shared memory
   //<-
   #if 1
   managed_shared_memory segment(create_only,
                                 test::get_process_id_name(),  //segment name
                                 65536);
   #else
   //->
   managed_shared_memory segment(create_only,
                                 "MySharedMemory",  //segment name
                                 65536);
   //<-
   #endif
   //->

   //Create a private_node_allocator that allocates ints from the managed segment
   //The number of chunks per segment is the default value
   typedef private_node_allocator<int, managed_shared_memory::segment_manager>
      private_node_allocator_t;
   private_node_allocator_t allocator_instance(segment.get_segment_manager());

   //Create another private_node_allocator.
   private_node_allocator_t allocator_instance2(segment.get_segment_manager());

   //Although the segment manager address
   //is the same, this private_node_allocator will have its own pool so
   //"allocator_instance2" CAN'T deallocate nodes allocated by "allocator_instance".
   //"allocator_instance2" is NOT equal to "allocator_instance"
   assert(allocator_instance != allocator_instance2);

   //Create another node_allocator using copy-constructor.
   private_node_allocator_t allocator_instance3(allocator_instance2);

   //This allocator is also unequal to allocator_instance2
   assert(allocator_instance2 != allocator_instance3);

   //Pools are destroyed with the allocators
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
