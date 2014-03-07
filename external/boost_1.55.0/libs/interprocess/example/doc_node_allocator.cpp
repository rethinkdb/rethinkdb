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
//[doc_node_allocator
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>
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
   } remover;
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

   //Create a node_allocator that allocates ints from the managed segment
   //The number of chunks per segment is the default value
   typedef node_allocator<int, managed_shared_memory::segment_manager>
      node_allocator_t;
   node_allocator_t allocator_instance(segment.get_segment_manager());

   //Create another node_allocator. Since the segment manager address
   //is the same, this node_allocator will be
   //attached to the same pool so "allocator_instance2" can deallocate
   //nodes allocated by "allocator_instance"
   node_allocator_t allocator_instance2(segment.get_segment_manager());

   //Create another node_allocator using copy-constructor. This
   //node_allocator will also be attached to the same pool
   node_allocator_t allocator_instance3(allocator_instance2);

   //All allocators are equal
   assert(allocator_instance == allocator_instance2);
   assert(allocator_instance2 == allocator_instance3);

   //So memory allocated with one can be deallocated with another
   allocator_instance2.deallocate(allocator_instance.allocate(1), 1);
   allocator_instance3.deallocate(allocator_instance2.allocate(1), 1);

   //The common pool will be destroyed here, since no allocator is
   //attached to the pool
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
