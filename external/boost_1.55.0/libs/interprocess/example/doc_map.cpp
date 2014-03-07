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
//[doc_map
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <utility>
//<-
#include "../test/get_process_id_name.hpp"
//->

int main ()
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

   //Shared memory front-end that is able to construct objects
   //associated with a c-string. Erase previous shared memory with the name
   //to be used and create the memory segment at the specified address and initialize resources
   //<-
   #if 1
   managed_shared_memory segment(create_only,test::get_process_id_name(), 65536);
   #else
   //->
   managed_shared_memory segment
      (create_only
      ,"MySharedMemory" //segment name
      ,65536);          //segment size in bytes
   //<-
   #endif
   //->

   //Note that map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>,
   //so the allocator must allocate that pair.
   typedef int    KeyType;
   typedef float  MappedType;
   typedef std::pair<const int, float> ValueType;

   //Alias an STL compatible allocator of for the map.
   //This allocator will allow to place containers
   //in managed shared memory segments
   typedef allocator<ValueType, managed_shared_memory::segment_manager>
      ShmemAllocator;

   //Alias a map of ints that uses the previous STL-like allocator.
   //Note that the third parameter argument is the ordering function
   //of the map, just like with std::map, used to compare the keys.
   typedef map<KeyType, MappedType, std::less<KeyType>, ShmemAllocator> MyMap;

   //Initialize the shared memory STL-compatible allocator
   ShmemAllocator alloc_inst (segment.get_segment_manager());

   //Construct a shared memory map.
   //Note that the first parameter is the comparison function,
   //and the second one the allocator.
   //This the same signature as std::map's constructor taking an allocator
   MyMap *mymap =
      segment.construct<MyMap>("MyMap")      //object name
                                 (std::less<int>() //first  ctor parameter
                                 ,alloc_inst);     //second ctor parameter

   //Insert data in the map
   for(int i = 0; i < 100; ++i){
      mymap->insert(std::pair<const int, float>(i, (float)i));
   }
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
