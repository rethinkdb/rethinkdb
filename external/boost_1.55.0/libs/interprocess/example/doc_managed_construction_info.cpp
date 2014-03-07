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
//[doc_managed_construction_info
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>
#include <cstring>
//<-
#include "../test/get_process_id_name.hpp"
//->

class my_class
{
   //...
};

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

   //<-
   #if 1
   managed_shared_memory managed_shm(create_only, test::get_process_id_name(), 10000*sizeof(std::size_t));
   #else
   //->
   managed_shared_memory managed_shm(create_only, "MySharedMemory", 10000*sizeof(std::size_t));
   //<-
   #endif
   //->

   //Construct objects
   my_class *named_object  = managed_shm.construct<my_class>("Object name")[1]();
   my_class *unique_object = managed_shm.construct<my_class>(unique_instance)[2]();
   my_class *anon_object   = managed_shm.construct<my_class>(anonymous_instance)[3]();

   //Now test "get_instance_name" function.
   assert(0 == std::strcmp(managed_shared_memory::get_instance_name(named_object), "Object name"));
   assert(0 == managed_shared_memory::get_instance_name(unique_object));
   assert(0 == managed_shared_memory::get_instance_name(anon_object));

   //Now test "get_instance_type" function.
   assert(named_type     == managed_shared_memory::get_instance_type(named_object));
   assert(unique_type    == managed_shared_memory::get_instance_type(unique_object));
   assert(anonymous_type == managed_shared_memory::get_instance_type(anon_object));

   //Now test "get_instance_length" function.
   assert(1 == managed_shared_memory::get_instance_length(named_object));
   assert(2 == managed_shared_memory::get_instance_length(unique_object));
   assert(3 == managed_shared_memory::get_instance_length(anon_object));

   managed_shm.destroy_ptr(named_object);
   managed_shm.destroy_ptr(unique_object);
   managed_shm.destroy_ptr(anon_object);
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
