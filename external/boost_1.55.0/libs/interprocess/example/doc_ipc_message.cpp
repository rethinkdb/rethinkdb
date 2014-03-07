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
//[doc_ipc_message
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib> //std::system
#include <sstream>
//<-
#include "../test/get_process_id_name.hpp"
//->

int main (int argc, char *argv[])
{
   using namespace boost::interprocess;
   if(argc == 1){  //Parent process
      //Remove shared memory on construction and destruction
      struct shm_remove
      {
      //<-
      #if 1
         shm_remove() {  shared_memory_object::remove(test::get_process_id_name()); }
         ~shm_remove(){  shared_memory_object::remove(test::get_process_id_name()); }
      #else
      //->
         shm_remove() {  shared_memory_object::remove("MySharedMemory"); }
         ~shm_remove(){  shared_memory_object::remove("MySharedMemory"); }
      //<-
      #endif
      //->
      } remover;
      //<-
      (void)remover;
      //->

      //Create a managed shared memory segment
      //<-
      #if 1
      managed_shared_memory segment(create_only, test::get_process_id_name(), 65536);
      #else
      //->
      managed_shared_memory segment(create_only, "MySharedMemory", 65536);
      //<-
      #endif
      //->

      //Allocate a portion of the segment (raw memory)
      managed_shared_memory::size_type free_memory = segment.get_free_memory();
      void * shptr = segment.allocate(1024/*bytes to allocate*/);

      //Check invariant
      if(free_memory <= segment.get_free_memory())
         return 1;

      //An handle from the base address can identify any byte of the shared
      //memory segment even if it is mapped in different base addresses
      managed_shared_memory::handle_t handle = segment.get_handle_from_address(shptr);
      std::stringstream s;
      s << argv[0] << " " << handle;
      //<-
      s << " " << test::get_process_id_name();
      //->
      s << std::ends;
      //Launch child process
      if(0 != std::system(s.str().c_str()))
         return 1;
      //Check memory has been freed
      if(free_memory != segment.get_free_memory())
         return 1;
   }
   else{
      //Open managed segment
      //<-
      #if 1
      managed_shared_memory segment(open_only, argv[2]);
      #else
      //->
      managed_shared_memory segment(open_only, "MySharedMemory");
      //<-
      #endif
      //->

      //An handle from the base address can identify any byte of the shared
      //memory segment even if it is mapped in different base addresses
      managed_shared_memory::handle_t handle = 0;

      //Obtain handle value
      std::stringstream s; s << argv[1]; s >> handle;

      //Get buffer local address from handle
      void *msg = segment.get_address_from_handle(handle);

      //Deallocate previously allocated memory
      segment.deallocate(msg);
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
