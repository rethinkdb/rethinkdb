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
//[doc_managed_external_buffer
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <cstring>
#include <boost/aligned_storage.hpp>

int main()
{
   using namespace boost::interprocess;

   //Create the static memory who will store all objects
   const int memsize = 65536;

   static boost::aligned_storage<memsize>::type static_buffer;

   //This managed memory will construct objects associated with
   //a wide string in the static buffer
   wmanaged_external_buffer objects_in_static_memory
      (create_only, &static_buffer, memsize);

   //We optimize resources to create 100 named objects in the static buffer
   objects_in_static_memory.reserve_named_objects(100);

   //Alias an integer node allocator type
   //This allocator will allocate memory inside the static buffer
   typedef allocator<int, wmanaged_external_buffer::segment_manager>
      allocator_t;

   //Alias a STL compatible list to be constructed in the static buffer
   typedef list<int, allocator_t>    MyBufferList;

   //The list must be initialized with the allocator
   //All objects created with objects_in_static_memory will
   //be stored in the static_buffer!
   MyBufferList *list = objects_in_static_memory.construct<MyBufferList>(L"MyList")
                           (objects_in_static_memory.get_segment_manager());
   //<-
   (void)list;
   //->
   //Since the allocation algorithm from wmanaged_external_buffer uses relative
   //pointers and all the pointers constructed int the static memory point
   //to objects in the same segment,  we can create another static buffer
   //from the first one and duplicate all the data.
   static boost::aligned_storage<memsize>::type static_buffer2;
   std::memcpy(&static_buffer2, &static_buffer, memsize);

   //Now open the duplicated managed memory passing the memory as argument
   wmanaged_external_buffer objects_in_static_memory2
      (open_only, &static_buffer2, memsize);

   //Check that "MyList" has been duplicated in the second buffer
   if(!objects_in_static_memory2.find<MyBufferList>(L"MyList").first)
      return 1;

   //Destroy the lists from the static buffers
   objects_in_static_memory.destroy<MyBufferList>(L"MyList");
   objects_in_static_memory2.destroy<MyBufferList>(L"MyList");
   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
