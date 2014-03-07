//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <algorithm>
#include <vector>
#include <list>
#include <iostream>
#include <functional>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/managed_heap_memory.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#include "print_container.hpp"

/******************************************************************************/
/*                                                                            */
/*  This example constructs repeats the same operations with std::list,       */
/*  shmem_list in user provided buffer, and shmem_list in heap memory         */
/*                                                                            */
/******************************************************************************/

using namespace boost::interprocess;

//We will work with wide characters for user memory objects
//Alias <integer> node allocator type
typedef node_allocator
   <int, wmanaged_external_buffer::segment_manager> user_node_allocator_t;
typedef node_allocator
   <int, wmanaged_heap_memory::segment_manager> heap_node_allocator_t;

//Alias list types
typedef list<int, user_node_allocator_t>    MyUserList;
typedef list<int, heap_node_allocator_t>    MyHeapList;
typedef std::list<int>                      MyStdList;

//Function to check if both lists are equal
bool CheckEqual(MyUserList *userlist, MyStdList *stdlist, MyHeapList *heaplist)
{
   return std::equal(userlist->begin(), userlist->end(), stdlist->begin()) &&
          std::equal(heaplist->begin(), heaplist->end(), stdlist->begin());
}

int main ()
{
   //Create the user memory who will store all objects
   const int size_aligner  = sizeof(::boost::detail::max_align);
   const int memsize       = 65536/size_aligner*size_aligner;
   static ::boost::detail::max_align static_buffer[memsize/size_aligner];

   {
      //Now test move semantics
      managed_heap_memory original(memsize);
      managed_heap_memory move_ctor(boost::move(original));
      managed_heap_memory move_assign;
      move_assign = boost::move(move_ctor);
      original.swap(move_assign);
   }
   {
      //Now test move semantics
      managed_external_buffer original(create_only, static_buffer, memsize);
      managed_external_buffer move_ctor(boost::move(original));
      managed_external_buffer move_assign;
      move_assign = boost::move(move_ctor);
      original.swap(move_assign);
   }

   //Named new capable user mem allocator
   wmanaged_external_buffer user_buffer(create_only, static_buffer, memsize);

   //Named new capable heap mem allocator
   wmanaged_heap_memory heap_buffer(memsize);

   //Test move semantics
   {
      wmanaged_external_buffer user_default;
      wmanaged_external_buffer temp_external(boost::move(user_buffer));
      user_default = boost::move(temp_external);
      user_buffer  = boost::move(user_default);
      wmanaged_heap_memory heap_default;
      wmanaged_heap_memory temp_heap(boost::move(heap_buffer));
      heap_default = boost::move(temp_heap);
      heap_buffer  = boost::move(heap_default);
   }

   //Initialize memory
   user_buffer.reserve_named_objects(100);
   heap_buffer.reserve_named_objects(100);

   //User memory allocator must be always be initialized
   //since it has no default constructor
   MyUserList *userlist = user_buffer.construct<MyUserList>(L"MyUserList")
                           (user_buffer.get_segment_manager());

   MyHeapList *heaplist = heap_buffer.construct<MyHeapList>(L"MyHeapList")
                           (heap_buffer.get_segment_manager());

   //Alias heap list
   MyStdList *stdlist = new MyStdList;

   int i;
   const int max = 100;
   for(i = 0; i < max; ++i){
      userlist->push_back(i);
      heaplist->push_back(i);
      stdlist->push_back(i);
   }
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   userlist->erase(userlist->begin()++);
   heaplist->erase(heaplist->begin()++);
   stdlist->erase(stdlist->begin()++);
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   userlist->pop_back();
   heaplist->pop_back();
   stdlist->pop_back();
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   userlist->pop_front();
   heaplist->pop_front();
   stdlist->pop_front();
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   std::vector<int> aux_vect;
   #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
   aux_vect.assign(50, -1);
   userlist->assign(aux_vect.begin(), aux_vect.end());
   heaplist->assign(aux_vect.begin(), aux_vect.end());
   stdlist->assign(aux_vect.begin(), aux_vect.end());
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;
   #endif

   userlist->sort();
   heaplist->sort();
   stdlist->sort();
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   #if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
   aux_vect.assign(50, 0);
   #endif
   userlist->insert(userlist->begin(), aux_vect.begin(), aux_vect.end());
   heaplist->insert(heaplist->begin(), aux_vect.begin(), aux_vect.end());
   stdlist->insert(stdlist->begin(), aux_vect.begin(), aux_vect.end());

   userlist->unique();
   heaplist->unique();
   stdlist->unique();
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   userlist->sort(std::greater<int>());
   heaplist->sort(std::greater<int>());
   stdlist->sort(std::greater<int>());
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   userlist->resize(userlist->size()/2);
   heaplist->resize(heaplist->size()/2);
   stdlist->resize(stdlist->size()/2);
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   userlist->remove(*userlist->begin());
   heaplist->remove(*heaplist->begin());
   stdlist->remove(*stdlist->begin());
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   for(i = 0; i < max; ++i){
      userlist->push_back(i);
      heaplist->push_back(i);
      stdlist->push_back(i);
   }

   MyUserList otheruserlist(*userlist);
   MyHeapList otherheaplist(*heaplist);
   MyStdList otherstdlist(*stdlist);
   userlist->splice(userlist->begin(), otheruserlist);
   heaplist->splice(heaplist->begin(), otherheaplist);
   stdlist->splice(stdlist->begin(), otherstdlist);
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   otheruserlist = *userlist;
   otherheaplist = *heaplist;
   otherstdlist = *stdlist;

   userlist->sort(std::greater<int>());
   heaplist->sort(std::greater<int>());
   stdlist->sort(std::greater<int>());
   otheruserlist.sort(std::greater<int>());
   otherheaplist.sort(std::greater<int>());
   otherstdlist.sort(std::greater<int>());
   userlist->merge(otheruserlist, std::greater<int>());
   heaplist->merge(otherheaplist, std::greater<int>());
   stdlist->merge(otherstdlist, std::greater<int>());
   if(!CheckEqual(userlist, stdlist, heaplist)) return 1;

   user_buffer.destroy<MyUserList>(L"MyUserList");
   delete stdlist;

   //Fill heap buffer until is full
   try{
      while(1){
         heaplist->insert(heaplist->end(), 0);
      }
   }
   catch(boost::interprocess::bad_alloc &){}

   MyHeapList::size_type heap_list_size = heaplist->size();

   //Copy heap buffer to another
   const char *insert_beg = static_cast<char*>(heap_buffer.get_address());
   const char *insert_end = insert_beg + heap_buffer.get_size();
   std::vector<char> grow_copy (insert_beg, insert_end);

   //Destroy old list
   heap_buffer.destroy<MyHeapList>(L"MyHeapList");

   //Resize copy buffer
   grow_copy.resize(memsize*2);

   //Open Interprocess machinery in the new managed external buffer
   wmanaged_external_buffer user_buffer2(open_only, &grow_copy[0], memsize);

   //Expand old Interprocess machinery to the new size
   user_buffer2.grow(memsize);

   //Get a pointer to the full list
   userlist = user_buffer2.find<MyUserList>(L"MyHeapList").first;
   if(!userlist){
      return 1;
   }

   //Fill user buffer until is full
   try{
      while(1){
         userlist->insert(userlist->end(), 0);
      }
   }
   catch(boost::interprocess::bad_alloc &){}

   MyUserList::size_type user_list_size = userlist->size();

   if(user_list_size <= heap_list_size){
      return 1;
   }

   user_buffer2.destroy_ptr(userlist);

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
