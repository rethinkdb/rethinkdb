////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
////////////////////////////////////////

#ifndef BOOST_CONTAINER_TEST_SET_TEST_HEADER
#define BOOST_CONTAINER_TEST_SET_TEST_HEADER

#include <boost/container/detail/config_begin.hpp>
#include "check_equal_containers.hpp"
#include <memory>
#include <set>
#include <functional>
#include "print_container.hpp"
#include <boost/move/utility.hpp>
#include <boost/move/iterator.hpp>
#include <string>

namespace boost{
namespace container {
namespace test{

template<class MyBoostSet
        ,class MyStdSet
        ,class MyBoostMultiSet
        ,class MyStdMultiSet>
int set_test ()
{
   typedef typename MyBoostSet::value_type IntType;
   const int max = 100;

   //Shared memory allocator must be always be initialized
   //since it has no default constructor
   MyBoostSet *boostset = new MyBoostSet;
   MyStdSet *stdset = new MyStdSet;
   MyBoostMultiSet *boostmultiset = new MyBoostMultiSet;
   MyStdMultiSet *stdmultiset = new MyStdMultiSet;

   //Test construction from a range  
   {
      IntType aux_vect[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(i/2);
         aux_vect[i] = boost::move(move_me);
      }
      int aux_vect2[50];
      for(int i = 0; i < 50; ++i){
         aux_vect2[i] = i/2;
      }
      IntType aux_vect3[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(i/2);
         aux_vect3[i] = boost::move(move_me);
      }

      MyBoostSet *boostset2 = new MyBoostSet
            ( boost::make_move_iterator(&aux_vect[0])
            , boost::make_move_iterator(aux_vect + 50));
      MyStdSet *stdset2 = new MyStdSet(aux_vect2, aux_vect2 + 50);
      MyBoostMultiSet *boostmultiset2 = new MyBoostMultiSet
            ( boost::make_move_iterator(&aux_vect3[0])
            , boost::make_move_iterator(aux_vect3 + 50));
      MyStdMultiSet *stdmultiset2 = new MyStdMultiSet(aux_vect2, aux_vect2 + 50);
      if(!CheckEqualContainers(boostset2, stdset2)){
         std::cout << "Error in construct<MyBoostSet>(MyBoostSet2)" << std::endl;
         return 1;
      }
      if(!CheckEqualContainers(boostmultiset2, stdmultiset2)){
         std::cout << "Error in construct<MyBoostMultiSet>(MyBoostMultiSet2)" << std::endl;
         return 1;
      }

      //ordered range insertion
      for(int i = 0; i < 50; ++i){
         IntType move_me(i);
         aux_vect[i] = boost::move(move_me);
      }

      for(int i = 0; i < 50; ++i){
         aux_vect2[i] = i;
      }

      for(int i = 0; i < 50; ++i){
         IntType move_me(i);
         aux_vect3[i] = boost::move(move_me);
      }

      MyBoostSet *boostset3 = new MyBoostSet
            ( ordered_unique_range
            , boost::make_move_iterator(&aux_vect[0])
            , boost::make_move_iterator(aux_vect + 50));
      MyStdSet *stdset3 = new MyStdSet(aux_vect2, aux_vect2 + 50);
      MyBoostMultiSet *boostmultiset3 = new MyBoostMultiSet
            ( ordered_range
            , boost::make_move_iterator(&aux_vect3[0])
            , boost::make_move_iterator(aux_vect3 + 50));
      MyStdMultiSet *stdmultiset3 = new MyStdMultiSet(aux_vect2, aux_vect2 + 50);

      if(!CheckEqualContainers(boostset3, stdset3)){
         std::cout << "Error in construct<MyBoostSet>(MyBoostSet3)" << std::endl;
         return 1;
      }
      if(!CheckEqualContainers(boostmultiset3, stdmultiset3)){
         std::cout << "Error in construct<MyBoostMultiSet>(MyBoostMultiSet3)" << std::endl;
         return 1;
      }

      delete boostset2;
      delete boostmultiset2;
      delete stdset2;
      delete stdmultiset2;
      delete boostset3;
      delete boostmultiset3;
      delete stdset3;
      delete stdmultiset3;
   }

   for(int i = 0; i < max; ++i){
      IntType move_me(i);
      boostset->insert(boost::move(move_me));
      stdset->insert(i);
      boostset->insert(IntType(i));
      stdset->insert(i);
      IntType move_me2(i);
      boostmultiset->insert(boost::move(move_me2));
      stdmultiset->insert(i);
      boostmultiset->insert(IntType(i));
      stdmultiset->insert(i);
   }

   if(!CheckEqualContainers(boostset, stdset)){
      std::cout << "Error in boostset->insert(boost::move(move_me)" << std::endl;
      return 1;
   }

   if(!CheckEqualContainers(boostmultiset, stdmultiset)){
      std::cout << "Error in boostmultiset->insert(boost::move(move_me)" << std::endl;
      return 1;
   }

   typename MyBoostSet::iterator it = boostset->begin();
   typename MyBoostSet::const_iterator cit = it;
   (void)cit;

   boostset->erase(boostset->begin());
   stdset->erase(stdset->begin());
   boostmultiset->erase(boostmultiset->begin());
   stdmultiset->erase(stdmultiset->begin());
   if(!CheckEqualContainers(boostset, stdset)){
      std::cout << "Error in boostset->erase(boostset->begin())" << std::endl;
      return 1;
   }
   if(!CheckEqualContainers(boostmultiset, stdmultiset)){
      std::cout << "Error in boostmultiset->erase(boostmultiset->begin())" << std::endl;
      return 1;
   }

   boostset->erase(boostset->begin());
   stdset->erase(stdset->begin());
   boostmultiset->erase(boostmultiset->begin());
   stdmultiset->erase(stdmultiset->begin());
   if(!CheckEqualContainers(boostset, stdset)){
      std::cout << "Error in boostset->erase(boostset->begin())" << std::endl;
      return 1;
   }
   if(!CheckEqualContainers(boostmultiset, stdmultiset)){
      std::cout << "Error in boostmultiset->erase(boostmultiset->begin())" << std::endl;
      return 1;
   }

   //Swapping test
   MyBoostSet tmpboosteset2;
   MyStdSet tmpstdset2;
   MyBoostMultiSet tmpboostemultiset2;
   MyStdMultiSet tmpstdmultiset2;
   boostset->swap(tmpboosteset2);
   stdset->swap(tmpstdset2);
   boostmultiset->swap(tmpboostemultiset2);
   stdmultiset->swap(tmpstdmultiset2);
   boostset->swap(tmpboosteset2);
   stdset->swap(tmpstdset2);
   boostmultiset->swap(tmpboostemultiset2);
   stdmultiset->swap(tmpstdmultiset2);
   if(!CheckEqualContainers(boostset, stdset)){
      std::cout << "Error in boostset->swap(tmpboosteset2)" << std::endl;
      return 1;
   }
   if(!CheckEqualContainers(boostmultiset, stdmultiset)){
      std::cout << "Error in boostmultiset->swap(tmpboostemultiset2)" << std::endl;
      return 1;
   }

   //Insertion from other container
   //Initialize values
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
      IntType aux_vect3[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect3[i] = boost::move(move_me);
      }

      boostset->insert(boost::make_move_iterator(&aux_vect[0]), boost::make_move_iterator(aux_vect + 50));
      stdset->insert(aux_vect2, aux_vect2 + 50);
      boostmultiset->insert(boost::make_move_iterator(&aux_vect3[0]), boost::make_move_iterator(aux_vect3 + 50));
      stdmultiset->insert(aux_vect2, aux_vect2 + 50);
      if(!CheckEqualContainers(boostset, stdset)){
         std::cout << "Error in boostset->insert(boost::make_move_iterator(&aux_vect[0])..." << std::endl;
         return 1;
      }
      if(!CheckEqualContainers(boostmultiset, stdmultiset)){
         std::cout << "Error in boostmultiset->insert(boost::make_move_iterator(&aux_vect3[0]), ..." << std::endl;
         return 1;
      }

      for(int i = 0, j = static_cast<int>(boostset->size()); i < j; ++i){
         IntType erase_me(i);
         boostset->erase(erase_me);
         stdset->erase(i);
         boostmultiset->erase(erase_me);
         stdmultiset->erase(i);
         if(!CheckEqualContainers(boostset, stdset)){
            std::cout << "Error in boostset->erase(erase_me)" << boostset->size() << " " << stdset->size() << std::endl;
            return 1;
         }
         if(!CheckEqualContainers(boostmultiset, stdmultiset)){
            std::cout << "Error in boostmultiset->erase(erase_me)" << std::endl;
            return 1;
         }
      }
   }
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
      IntType aux_vect3[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect3[i] = boost::move(move_me);
      }

      IntType aux_vect4[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect4[i] = boost::move(move_me);
      }

      IntType aux_vect5[50];
      for(int i = 0; i < 50; ++i){
         IntType move_me(-1);
         aux_vect5[i] = boost::move(move_me);
      }

      boostset->insert(boost::make_move_iterator(&aux_vect[0]), boost::make_move_iterator(aux_vect + 50));
      boostset->insert(boost::make_move_iterator(&aux_vect3[0]), boost::make_move_iterator(aux_vect3 + 50));
      stdset->insert(aux_vect2, aux_vect2 + 50);
      stdset->insert(aux_vect2, aux_vect2 + 50);
      boostmultiset->insert(boost::make_move_iterator(&aux_vect4[0]), boost::make_move_iterator(aux_vect4 + 50));
      boostmultiset->insert(boost::make_move_iterator(&aux_vect5[0]), boost::make_move_iterator(aux_vect5 + 50));
      stdmultiset->insert(aux_vect2, aux_vect2 + 50);
      stdmultiset->insert(aux_vect2, aux_vect2 + 50);
      if(!CheckEqualContainers(boostset, stdset)){
         std::cout << "Error in boostset->insert(boost::make_move_iterator(&aux_vect3[0])..." << std::endl;
         return 1;
      }
      if(!CheckEqualContainers(boostmultiset, stdmultiset)){
         std::cout << "Error in boostmultiset->insert(boost::make_move_iterator(&aux_vect5[0])..." << std::endl;
         return 1;
      }

      boostset->erase(*boostset->begin());
      stdset->erase(*stdset->begin());
      boostmultiset->erase(*boostmultiset->begin());
      stdmultiset->erase(*stdmultiset->begin());
      if(!CheckEqualContainers(boostset, stdset)){
         std::cout << "Error in boostset->erase(*boostset->begin())" << std::endl;
         return 1;
      }
      if(!CheckEqualContainers(boostmultiset, stdmultiset)){
         std::cout << "Error in boostmultiset->erase(*boostmultiset->begin())" << std::endl;
         return 1;
      }
   }

   for(int i = 0; i < max; ++i){
      IntType move_me(i);
      boostset->insert(boost::move(move_me));
      stdset->insert(i);
      IntType move_me2(i);
      boostmultiset->insert(boost::move(move_me2));
      stdmultiset->insert(i);
   }

   if(!CheckEqualContainers(boostset, stdset)){
      std::cout << "Error in boostset->insert(boost::move(move_me)) try 2" << std::endl;
      return 1;
   }
   if(!CheckEqualContainers(boostmultiset, stdmultiset)){
      std::cout << "Error in boostmultiset->insert(boost::move(move_me2)) try 2" << std::endl;
      return 1;
   }

   for(int i = 0; i < max; ++i){
      {
         IntType move_me(i);
         boostset->insert(boostset->begin(), boost::move(move_me));
         stdset->insert(stdset->begin(), i);
         //PrintContainers(boostset, stdset);
         IntType move_me2(i);
         boostmultiset->insert(boostmultiset->begin(), boost::move(move_me2));
         stdmultiset->insert(stdmultiset->begin(), i);
         //PrintContainers(boostmultiset, stdmultiset);
         if(!CheckEqualContainers(boostset, stdset)){
            std::cout << "Error in boostset->insert(boostset->begin(), boost::move(move_me))" << std::endl;
            return 1;
         }
         if(!CheckEqualContainers(boostmultiset, stdmultiset)){
            std::cout << "Error in boostmultiset->insert(boostmultiset->begin(), boost::move(move_me2))" << std::endl;
            return 1;
         }

         IntType move_me3(i);
         boostset->insert(boostset->end(), boost::move(move_me3));
         stdset->insert(stdset->end(), i);
         IntType move_me4(i);
         boostmultiset->insert(boostmultiset->end(), boost::move(move_me4));
         stdmultiset->insert(stdmultiset->end(), i);
         if(!CheckEqualContainers(boostset, stdset)){
            std::cout << "Error in boostset->insert(boostset->end(), boost::move(move_me3))" << std::endl;
            return 1;
         }
         if(!CheckEqualContainers(boostmultiset, stdmultiset)){
            std::cout << "Error in boostmultiset->insert(boostmultiset->end(), boost::move(move_me4))" << std::endl;
            return 1;
         }
      }
      {
      IntType move_me(i);
      boostset->insert(boostset->upper_bound(move_me), boost::move(move_me));
      stdset->insert(stdset->upper_bound(i), i);
      //PrintContainers(boostset, stdset);
      IntType move_me2(i);
      boostmultiset->insert(boostmultiset->upper_bound(move_me2), boost::move(move_me2));
      stdmultiset->insert(stdmultiset->upper_bound(i), i);
      //PrintContainers(boostmultiset, stdmultiset);
      if(!CheckEqualContainers(boostset, stdset)){
         std::cout << "Error in boostset->insert(boostset->upper_bound(move_me), boost::move(move_me))" << std::endl;
         return 1;
      }
      if(!CheckEqualContainers(boostmultiset, stdmultiset)){
         std::cout << "Error in boostmultiset->insert(boostmultiset->upper_bound(move_me2), boost::move(move_me2))" << std::endl;
         return 1;
      }

      }
      {
      IntType move_me(i);
      IntType move_me2(i);
      boostset->insert(boostset->lower_bound(move_me), boost::move(move_me2));
      stdset->insert(stdset->lower_bound(i), i);
      //PrintContainers(boostset, stdset);
      move_me2 = i;
      boostmultiset->insert(boostmultiset->lower_bound(move_me2), boost::move(move_me2));
      stdmultiset->insert(stdmultiset->lower_bound(i), i);
      //PrintContainers(boostmultiset, stdmultiset);
      if(!CheckEqualContainers(boostset, stdset)){
         std::cout << "Error in boostset->insert(boostset->lower_bound(move_me), boost::move(move_me2))" << std::endl;
         return 1;
      }
      if(!CheckEqualContainers(boostmultiset, stdmultiset)){
         std::cout << "Error in boostmultiset->insert(boostmultiset->lower_bound(move_me2), boost::move(move_me2))" << std::endl;
         return 1;
      }
      }
   }

   //Compare count with std containers
   for(int i = 0; i < max; ++i){
      IntType count_me(i);
      if(boostset->count(count_me) != stdset->count(i)){
         return -1;
      }
      if(boostmultiset->count(count_me) != stdmultiset->count(i)){
         return -1;
      }
   }

   //Now do count exercise
   boostset->erase(boostset->begin(), boostset->end());
   boostmultiset->erase(boostmultiset->begin(), boostmultiset->end());
   boostset->clear();
   boostmultiset->clear();

   for(int j = 0; j < 3; ++j)
   for(int i = 0; i < 100; ++i){
      IntType move_me(i);
      boostset->insert(boost::move(move_me));
      IntType move_me2(i);
      boostmultiset->insert(boost::move(move_me2));
      IntType count_me(i);
      if(boostset->count(count_me) != typename MyBoostMultiSet::size_type(1)){
         std::cout << "Error in boostset->count(count_me)" << std::endl;
         return 1;
      }
      if(boostmultiset->count(count_me) != typename MyBoostMultiSet::size_type(j+1)){
         std::cout << "Error in boostmultiset->count(count_me)" << std::endl;
         return 1;
      }
   }

   delete boostset;
   delete stdset;
   delete boostmultiset;
   delete stdmultiset;
   return 0;
}

template<class MyBoostSet
        ,class MyStdSet
        ,class MyBoostMultiSet
        ,class MyStdMultiSet>
int set_test_copyable ()
{
   typedef typename MyBoostSet::value_type IntType;
   const int max = 100;

   BOOST_TRY{
      //Shared memory allocator must be always be initialized
      //since it has no default constructor
      MyBoostSet *boostset = new MyBoostSet;
      MyStdSet *stdset = new MyStdSet;
      MyBoostMultiSet *boostmultiset = new MyBoostMultiSet;
      MyStdMultiSet *stdmultiset = new MyStdMultiSet;

      for(int i = 0; i < max; ++i){
         IntType move_me(i);
         boostset->insert(boost::move(move_me));
         stdset->insert(i);
         IntType move_me2(i);
         boostmultiset->insert(boost::move(move_me2));
         stdmultiset->insert(i);
      }
      if(!CheckEqualContainers(boostset, stdset)) return 1;
      if(!CheckEqualContainers(boostmultiset, stdmultiset)) return 1;

      {
         //Now, test copy constructor
         MyBoostSet boostsetcopy(*boostset);
         MyStdSet stdsetcopy(*stdset);

         if(!CheckEqualContainers(&boostsetcopy, &stdsetcopy))
            return 1;

         MyBoostMultiSet boostmsetcopy(*boostmultiset);
         MyStdMultiSet stdmsetcopy(*stdmultiset);

         if(!CheckEqualContainers(&boostmsetcopy, &stdmsetcopy))
            return 1;

         //And now assignment
         boostsetcopy  = *boostset;
         stdsetcopy  = *stdset;

         if(!CheckEqualContainers(&boostsetcopy, &stdsetcopy))
            return 1;

         boostmsetcopy = *boostmultiset;
         stdmsetcopy = *stdmultiset;
        
         if(!CheckEqualContainers(&boostmsetcopy, &stdmsetcopy))
            return 1;
      }
      delete boostset;
      delete boostmultiset;
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
