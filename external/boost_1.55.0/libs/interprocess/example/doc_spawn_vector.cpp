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
//[doc_spawn_vector
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <cstdlib> //std::system
//<-
#include "../test/get_process_id_name.hpp"
//->

using namespace boost::interprocess;

//Define an STL compatible allocator of ints that allocates from the managed_shared_memory.
//This allocator will allow placing containers in the segment
typedef allocator<int, managed_shared_memory::segment_manager>  ShmemAllocator;

//Alias a vector that uses the previous STL-like allocator so that allocates
//its values from the segment
typedef vector<int, ShmemAllocator> MyVector;

//Main function. For parent process argc == 1, for child process argc == 2
int main(int argc, char *argv[])
{
   if(argc == 1){ //Parent process
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

      //Create a new segment with given name and size
      //<-
      #if 1
      managed_shared_memory segment(create_only, test::get_process_id_name(), 65536);
      #else
      //->
      managed_shared_memory segment(create_only, "MySharedMemory", 65536);
      //<-
      #endif
      //->

      //Initialize shared memory STL-compatible allocator
      const ShmemAllocator alloc_inst (segment.get_segment_manager());

      //Construct a vector named "MyVector" in shared memory with argument alloc_inst
      MyVector *myvector = segment.construct<MyVector>("MyVector")(alloc_inst);

      for(int i = 0; i < 100; ++i)  //Insert data in the vector
         myvector->push_back(i);

      //Launch child process
      std::string s(argv[0]); s += " child ";
      //<-
      s += test::get_process_id_name();
      //->
      if(0 != std::system(s.c_str()))
         return 1;

      //Check child has destroyed the vector
      if(segment.find<MyVector>("MyVector").first)
         return 1;
   }
   else{ //Child process
      //Open the managed segment
      //<-
      #if 1
      managed_shared_memory segment(open_only, argv[2]);
      #else
      //->
      managed_shared_memory segment(open_only, "MySharedMemory");
      //<-
      #endif
      //->

      //Find the vector using the c-string name
      MyVector *myvector = segment.find<MyVector>("MyVector").first;

      //Use vector in reverse order
      std::sort(myvector->rbegin(), myvector->rend());

      //When done, destroy the vector from the segment
      segment.destroy<MyVector>("MyVector");
   }

   return 0;
};

//]
#include <boost/interprocess/detail/config_end.hpp>
