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
//[doc_managed_heap_memory
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/managed_heap_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <cstddef>

using namespace boost::interprocess;
typedef list<int, allocator<int, managed_heap_memory::segment_manager> >
   MyList;

int main ()
{
   //We will create a buffer of 1000 bytes to store a list
   managed_heap_memory heap_memory(1000);

   MyList * mylist = heap_memory.construct<MyList>("MyList")
                        (heap_memory.get_segment_manager());

   //Obtain handle, that identifies the list in the buffer
   managed_heap_memory::handle_t list_handle = heap_memory.get_handle_from_address(mylist);

   //Fill list until there is no more memory in the buffer
   try{
      while(1) {
         mylist->insert(mylist->begin(), 0);
      }
   }
   catch(const bad_alloc &){
      //memory is full
   }
   //Let's obtain the size of the list
   MyList::size_type old_size = mylist->size();

   //To make the list bigger, let's increase the heap buffer
   //in 1000 bytes more.
   heap_memory.grow(1000);

   //If memory has been reallocated, the old pointer is invalid, so
   //use previously obtained handle to find the new pointer.
   mylist = static_cast<MyList *>
               (heap_memory.get_address_from_handle(list_handle));

   //Fill list until there is no more memory in the buffer
   try{
      while(1) {
         mylist->insert(mylist->begin(), 0);
      }
   }
   catch(const bad_alloc &){
      //memory is full
   }

   //Let's obtain the new size of the list
   MyList::size_type new_size = mylist->size();

   assert(new_size > old_size);

   //Destroy list
   heap_memory.destroy_ptr(mylist);

   return 0;
}
//]
#include <boost/interprocess/detail/config_end.hpp>
