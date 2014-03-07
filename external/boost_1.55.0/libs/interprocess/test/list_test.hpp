////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_LIST_TEST_HEADER
#define BOOST_INTERPROCESS_TEST_LIST_TEST_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include "check_equal_containers.hpp"
#include <memory>
#include <list>
#include <vector>
#include <functional>
#include "print_container.hpp"
#include <boost/interprocess/detail/move.hpp>
#include <string>
#include "get_process_id_name.hpp"

namespace boost{
namespace interprocess{
namespace test{

template<bool DoublyLinked>
struct push_data_function
{
   template<class MyShmList, class MyStdList>
   static int execute(int max, MyShmList *shmlist, MyStdList *stdlist)
   {
      typedef typename MyShmList::value_type IntType;
      for(int i = 0; i < max; ++i){
         IntType move_me(i);
         shmlist->push_back(boost::move(move_me));
         stdlist->push_back(i);
         shmlist->push_back(IntType(i));
         stdlist->push_back(int(i));
      }
      if(!CheckEqualContainers(shmlist, stdlist))
         return 1;
      return 0;
   }
};

template<>
struct push_data_function<false>
{
   template<class MyShmList, class MyStdList>
   static int execute(int max, MyShmList *shmlist, MyStdList *stdlist)
   {
      typedef typename MyShmList::value_type IntType;
      for(int i = 0; i < max; ++i){
         IntType move_me(i);
         shmlist->push_front(boost::move(move_me));
         stdlist->push_front(i);
         shmlist->push_front(IntType(i));
         stdlist->push_front(int(i));
      }
      if(!CheckEqualContainers(shmlist, stdlist))
         return 1;
      return 0;
   }
};

template<bool DoublyLinked>
struct pop_back_function
{
   template<class MyStdList, class MyShmList>
   static int execute(MyShmList *shmlist, MyStdList *stdlist)
   {
      shmlist->pop_back();
      stdlist->pop_back();
      if(!CheckEqualContainers(shmlist, stdlist))
         return 1;
      return 0;
   }
};

template<>
struct pop_back_function<false>
{
   template<class MyStdList, class MyShmList>
   static int execute(MyShmList *shmlist, MyStdList *stdlist)
   {
      (void)shmlist; (void)stdlist;
      return 0;
   }
};

template<class ManagedSharedMemory
        ,class MyShmList
        ,bool  DoublyLinked>
int list_test (bool copied_allocators_equal = true)
{
   typedef std::list<int> MyStdList;
   typedef typename MyShmList::value_type IntType;
   const int memsize = 65536;
   const char *const shMemName = test::get_process_id_name();
   const int max = 100;
   typedef push_data_function<DoublyLinked> push_data_t;

   try{
      //Named new capable shared mem allocator
      //Create shared memory
      shared_memory_object::remove(shMemName);
      ManagedSharedMemory segment(create_only, shMemName, memsize);

      segment.reserve_named_objects(100);

      //Shared memory allocator must be always be initialized
      //since it has no default constructor
      MyShmList *shmlist = segment.template construct<MyShmList>("MyList")
                              (segment.get_segment_manager());


      MyStdList *stdlist = new MyStdList;

      if(push_data_t::execute(max/2, shmlist, stdlist)){
         return 1;
      }

      shmlist->erase(shmlist->begin()++);
      stdlist->erase(stdlist->begin()++);
      if(!CheckEqualContainers(shmlist, stdlist)) return 1;

      if(pop_back_function<DoublyLinked>::execute(shmlist, stdlist)){
         return 1;
      }

      shmlist->pop_front();
      stdlist->pop_front();
      if(!CheckEqualContainers(shmlist, stdlist)) return 1;

      {
         IntType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            IntType move_me(-1);
            aux_vect[i] = boost::move(move_me);
         }
         int aux_vect2[50];
         for(int i = 0; i < 50; ++i){
            aux_vect2[i] = -1;
         }
         shmlist->assign(::boost::make_move_iterator(&aux_vect[0])
                        ,::boost::make_move_iterator(&aux_vect[50]));
         stdlist->assign(&aux_vect2[0], &aux_vect2[50]);
         if(!CheckEqualContainers(shmlist, stdlist)) return 1;
      }

      if(copied_allocators_equal){
         shmlist->sort();
         stdlist->sort();
         if(!CheckEqualContainers(shmlist, stdlist)) return 1;
      }

      shmlist->reverse();
      stdlist->reverse();
      if(!CheckEqualContainers(shmlist, stdlist)) return 1;

      shmlist->reverse();
      stdlist->reverse();
      if(!CheckEqualContainers(shmlist, stdlist)) return 1;

      {
         IntType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            IntType move_me(-1);
            aux_vect[i] = boost::move(move_me);
         }
         int aux_vect2[50];
         for(int i = 0; i < 50; ++i){
            aux_vect2[i] = -1;
         }
         shmlist->insert(shmlist->begin()
                        ,::boost::make_move_iterator(&aux_vect[0])
                        ,::boost::make_move_iterator(&aux_vect[50]));
         stdlist->insert(stdlist->begin(), &aux_vect2[0], &aux_vect2[50]);
      }

      shmlist->unique();
      stdlist->unique();
      if(!CheckEqualContainers(shmlist, stdlist))
         return 1;

      if(copied_allocators_equal){
         shmlist->sort(std::greater<IntType>());
         stdlist->sort(std::greater<int>());
         if(!CheckEqualContainers(shmlist, stdlist))
            return 1;
      }

      shmlist->resize(25);
      stdlist->resize(25);
      shmlist->resize(50);
      stdlist->resize(50);
      shmlist->resize(0);
      stdlist->resize(0);
      if(!CheckEqualContainers(shmlist, stdlist))
         return 1;

      if(push_data_t::execute(max/2, shmlist, stdlist)){
         return 1;
      }
      {
         MyShmList othershmlist(shmlist->get_allocator());
         MyStdList otherstdlist;

         int listsize = (int)shmlist->size();

         if(push_data_t::execute(listsize/2, shmlist, stdlist)){
            return 1;
         }

         if(copied_allocators_equal){
            shmlist->splice(shmlist->begin(), othershmlist);
            stdlist->splice(stdlist->begin(), otherstdlist);
            if(!CheckEqualContainers(shmlist, stdlist))
               return 1;
         }

         listsize = (int)shmlist->size();

         if(push_data_t::execute(listsize/2, shmlist, stdlist)){
            return 1;
         }

         if(push_data_t::execute(listsize/2, &othershmlist, &otherstdlist)){
            return 1;
         }

         if(copied_allocators_equal){
            shmlist->sort(std::greater<IntType>());
            stdlist->sort(std::greater<int>());
            if(!CheckEqualContainers(shmlist, stdlist))
               return 1;

            othershmlist.sort(std::greater<IntType>());
            otherstdlist.sort(std::greater<int>());
            if(!CheckEqualContainers(&othershmlist, &otherstdlist))
               return 1;

            shmlist->merge(othershmlist, std::greater<IntType>());
            stdlist->merge(otherstdlist, std::greater<int>());
            if(!CheckEqualContainers(shmlist, stdlist))
               return 1;
         }

         for(int i = 0; i < max; ++i){
            shmlist->insert(shmlist->begin(), IntType(i));
            stdlist->insert(stdlist->begin(), int(i));
         }
         if(!CheckEqualContainers(shmlist, stdlist))
            return 1;
      }

      segment.template destroy<MyShmList>("MyList");
      delete stdlist;
      segment.shrink_to_fit_indexes();

      if(!segment.all_memory_deallocated())
         return 1;
   }
   catch(...){
      shared_memory_object::remove(shMemName);
      throw;
   }
   shared_memory_object::remove(shMemName);
   return 0;
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
