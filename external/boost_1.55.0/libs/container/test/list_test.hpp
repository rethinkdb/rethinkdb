////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
////////////////////////////////////////

#ifndef BOOST_CONTAINER_TEST_LIST_TEST_HEADER
#define BOOST_CONTAINER_TEST_LIST_TEST_HEADER

#include <boost/container/detail/config_begin.hpp>
#include "check_equal_containers.hpp"
#include <memory>
#include <list>
#include <vector>
#include <functional>
#include "print_container.hpp"
#include "input_from_forward_iterator.hpp"
#include <boost/move/utility.hpp>
#include <boost/move/iterator.hpp>
#include <string>

namespace boost{
namespace container {
namespace test{

template<class V1, class V2>
bool list_copyable_only(V1 *, V2 *, boost::container::container_detail::false_type)
{
   return true;
}

//Function to check if both sets are equal
template<class V1, class V2>
bool list_copyable_only(V1 *boostlist, V2 *stdlist, boost::container::container_detail::true_type)
{
   typedef typename V1::value_type IntType;
   boostlist->insert(boostlist->end(), 50, IntType(1));
   stdlist->insert(stdlist->end(), 50, 1);
   if(!test::CheckEqualContainers(boostlist, stdlist)) return false;

   {
      IntType move_me(1);
      boostlist->insert(boostlist->begin(), 50, boost::move(move_me));
      stdlist->insert(stdlist->begin(), 50, 1);
      if(!test::CheckEqualContainers(boostlist, stdlist)) return false;
   }
   {
      IntType move_me(2);
      boostlist->assign(boostlist->size()/2, boost::move(move_me));
      stdlist->assign(stdlist->size()/2, 2);
      if(!test::CheckEqualContainers(boostlist, stdlist)) return false;
   }
   {
      IntType move_me(3);
      boostlist->assign(boostlist->size()*3-1, boost::move(move_me));
      stdlist->assign(stdlist->size()*3-1, 3);
      if(!test::CheckEqualContainers(boostlist, stdlist)) return false;
   }

   {
      IntType copy_me(3);
      const IntType ccopy_me(3);
      boostlist->push_front(copy_me);
      stdlist->push_front(int(3));
      boostlist->push_front(ccopy_me);
      stdlist->push_front(int(3));
      if(!test::CheckEqualContainers(boostlist, stdlist)) return false;
   }

   return true;
}

template<bool DoublyLinked>
struct list_push_data_function
{
   template<class MyBoostList, class MyStdList>
   static int execute(int max, MyBoostList *boostlist, MyStdList *stdlist)
   {
      typedef typename MyBoostList::value_type IntType;
      for(int i = 0; i < max; ++i){
         IntType move_me(i);
         boostlist->push_back(boost::move(move_me));
         stdlist->push_back(i);
         boostlist->push_front(IntType(i));
         stdlist->push_front(int(i));
      }
      if(!CheckEqualContainers(boostlist, stdlist))
         return 1;
      return 0;
   }
};

template<>
struct list_push_data_function<false>
{
   template<class MyBoostList, class MyStdList>
   static int execute(int max, MyBoostList *boostlist, MyStdList *stdlist)
   {
      typedef typename MyBoostList::value_type IntType;
      for(int i = 0; i < max; ++i){
         IntType move_me(i);
         boostlist->push_front(boost::move(move_me));
         stdlist->push_front(i);
         boostlist->push_front(IntType(i));
         stdlist->push_front(int(i));
      }
      if(!CheckEqualContainers(boostlist, stdlist))
         return 1;
      return 0;
   }
};

template<bool DoublyLinked>
struct list_pop_back_function
{
   template<class MyStdList, class MyBoostList>
   static int execute(MyBoostList *boostlist, MyStdList *stdlist)
   {
      boostlist->pop_back();
      stdlist->pop_back();
      if(!CheckEqualContainers(boostlist, stdlist))
         return 1;
      return 0;
   }
};

template<>
struct list_pop_back_function<false>
{
   template<class MyStdList, class MyBoostList>
   static int execute(MyBoostList *boostlist, MyStdList *stdlist)
   {
      (void)boostlist; (void)stdlist;
      return 0;
   }
};

template<class MyBoostList
        ,bool  DoublyLinked>
int list_test (bool copied_allocators_equal = true)
{
   typedef std::list<int> MyStdList;
   typedef typename MyBoostList::value_type IntType;
   const int max = 100;
   typedef list_push_data_function<DoublyLinked> push_data_t;

   BOOST_TRY{
      MyBoostList *boostlist = new MyBoostList;
      MyStdList *stdlist = new MyStdList;

      if(push_data_t::execute(max, boostlist, stdlist)){
         return 1;
      }

      boostlist->erase(boostlist->begin()++);
      stdlist->erase(stdlist->begin()++);
      if(!CheckEqualContainers(boostlist, stdlist)) return 1;

      if(list_pop_back_function<DoublyLinked>::execute(boostlist, stdlist)){
         return 1;
      }

      boostlist->pop_front();
      stdlist->pop_front();
      if(!CheckEqualContainers(boostlist, stdlist)) return 1;

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
         boostlist->assign(boost::make_move_iterator(&aux_vect[0])
                          ,boost::make_move_iterator(&aux_vect[50]));
         stdlist->assign(&aux_vect2[0], &aux_vect2[50]);
         if(!CheckEqualContainers(boostlist, stdlist)) return 1;

         for(int i = 0; i < 50; ++i){
            IntType move_me(-1);
            aux_vect[i] = boost::move(move_me);
         }

         for(int i = 0; i < 50; ++i){
            aux_vect2[i] = -1;
         }
         boostlist->assign(boost::make_move_iterator(make_input_from_forward_iterator(&aux_vect[0]))
                          ,boost::make_move_iterator(make_input_from_forward_iterator(&aux_vect[50])));
         stdlist->assign(&aux_vect2[0], &aux_vect2[50]);
         if(!CheckEqualContainers(boostlist, stdlist)) return 1;
      }

      if(copied_allocators_equal){
         boostlist->sort();
         stdlist->sort();
         if(!CheckEqualContainers(boostlist, stdlist)) return 1;
      }

      boostlist->reverse();
      stdlist->reverse();
      if(!CheckEqualContainers(boostlist, stdlist)) return 1;

      boostlist->reverse();
      stdlist->reverse();
      if(!CheckEqualContainers(boostlist, stdlist)) return 1;

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
         typename MyBoostList::iterator old_begin = boostlist->begin();
         typename MyBoostList::iterator it_insert =
            boostlist->insert(boostlist->begin()
                        ,boost::make_move_iterator(&aux_vect[0])
                        ,boost::make_move_iterator(&aux_vect[50]));
         if(it_insert != boostlist->begin() || std::distance(it_insert, old_begin) != 50)
            return 1;

         stdlist->insert(stdlist->begin(), &aux_vect2[0], &aux_vect2[50]);
         if(!CheckEqualContainers(boostlist, stdlist))
            return 1;

         for(int i = 0; i < 50; ++i){
            IntType move_me(-1);
            aux_vect[i] = boost::move(move_me);
         }

         for(int i = 0; i < 50; ++i){
            aux_vect2[i] = -1;
         }

         old_begin = boostlist->begin();
         it_insert = boostlist->insert(boostlist->end()
                        ,boost::make_move_iterator(make_input_from_forward_iterator(&aux_vect[0]))
                        ,boost::make_move_iterator(make_input_from_forward_iterator(&aux_vect[50])));
         if(std::distance(it_insert, boostlist->end()) != 50)
            return 1;
         stdlist->insert(stdlist->end(), &aux_vect2[0], &aux_vect2[50]);
         if(!CheckEqualContainers(boostlist, stdlist))
            return 1;
      }

      boostlist->unique();
      stdlist->unique();
      if(!CheckEqualContainers(boostlist, stdlist))
         return 1;

      if(copied_allocators_equal){
         boostlist->sort(std::greater<IntType>());
         stdlist->sort(std::greater<int>());
         if(!CheckEqualContainers(boostlist, stdlist))
            return 1;
      }

      for(int i = 0; i < max; ++i){
         IntType new_int(i);
         boostlist->insert(boostlist->end(), boost::move(new_int));
         stdlist->insert(stdlist->end(), i);
         if(!test::CheckEqualContainers(boostlist, stdlist)) return 1;
      }
      if(!test::CheckEqualContainers(boostlist, stdlist)) return 1;

      boostlist->resize(25);
      stdlist->resize(25);
      boostlist->resize(50);
      stdlist->resize(50);
      boostlist->resize(0);
      stdlist->resize(0);
      if(!CheckEqualContainers(boostlist, stdlist))
         return 1;

      if(push_data_t::execute(max, boostlist, stdlist)){
         return 1;
      }
      {
         MyBoostList otherboostlist(boostlist->get_allocator());
         MyStdList otherstdlist;

         int listsize = (int)boostlist->size();

         if(push_data_t::execute(listsize, boostlist, stdlist)){
            return 1;
         }

         if(copied_allocators_equal){
            boostlist->splice(boostlist->begin(), otherboostlist);
            stdlist->splice(stdlist->begin(), otherstdlist);
            if(!CheckEqualContainers(boostlist, stdlist))
               return 1;  
         }

         listsize = (int)boostlist->size();

         if(push_data_t::execute(listsize, boostlist, stdlist)){
            return 1;
         }

         if(push_data_t::execute(listsize, &otherboostlist, &otherstdlist)){
            return 1;
         }

         if(copied_allocators_equal){
            boostlist->sort(std::greater<IntType>());
            stdlist->sort(std::greater<int>());
            if(!CheckEqualContainers(boostlist, stdlist))
               return 1;

            otherboostlist.sort(std::greater<IntType>());
            otherstdlist.sort(std::greater<int>());
            if(!CheckEqualContainers(&otherboostlist, &otherstdlist))
               return 1;

            boostlist->merge(otherboostlist, std::greater<IntType>());
            stdlist->merge(otherstdlist, std::greater<int>());
            if(!CheckEqualContainers(boostlist, stdlist))
               return 1;
         }

         if(!list_copyable_only(boostlist, stdlist
                        ,container_detail::bool_<boost::container::test::is_copyable<IntType>::value>())){
            return 1;
         }
      }

      delete boostlist;
      delete stdlist;
   }
   BOOST_CATCH(...){
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return 0;
}

}  //namespace test{
}  //namespace container {
}  //namespace boost{

#include <boost/container/detail/config_end.hpp>

#endif
