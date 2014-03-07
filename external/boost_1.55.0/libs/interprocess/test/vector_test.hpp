//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2004-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_VECTOR_TEST_HEADER
#define BOOST_INTERPROCESS_TEST_VECTOR_TEST_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <algorithm>
#include <memory>
#include <vector>
#include <iostream>
#include <functional>
#include <list>

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include "print_container.hpp"
#include "check_equal_containers.hpp"
#include "movable_int.hpp"
#include <string>
#include <vector>
#include "get_process_id_name.hpp"
#include "emplace_test.hpp"

namespace boost{
namespace interprocess{
namespace test{

template<class V1, class V2>
bool copyable_only(V1 *, V2 *, boost::interprocess::ipcdetail::false_type)
{
   return true;
}

//Function to check if both sets are equal
template<class V1, class V2>
bool copyable_only(V1 *shmvector, V2 *stdvector, boost::interprocess::ipcdetail::true_type)
{
   typedef typename V1::value_type IntType;
   std::size_t size = shmvector->size();
   stdvector->insert(stdvector->end(), 50, 1);
   shmvector->insert(shmvector->end(), 50, IntType(1));
   if(!test::CheckEqualContainers(shmvector, stdvector)) return false;

   {
      IntType move_me(1);
      stdvector->insert(stdvector->begin()+size/2, 50, 1);
      shmvector->insert(shmvector->begin()+size/2, 50, boost::move(move_me));
      if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
   }
   {
      IntType move_me(2);
      shmvector->assign(shmvector->size()/2, boost::move(move_me));
      stdvector->assign(stdvector->size()/2, 2);
      if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
   }
   {
      IntType move_me(3);
      shmvector->assign(shmvector->size()*3-1, boost::move(move_me));
      stdvector->assign(stdvector->size()*3-1, 3);
      if(!test::CheckEqualContainers(shmvector, stdvector)) return false;
   }
   return true;
}

template<class ManagedSharedMemory
        ,class MyShmVector>
int vector_test()
{
   typedef std::vector<int>                     MyStdVector;
   typedef typename MyShmVector::value_type     IntType;

   std::string process_name;
   test::get_process_id_name(process_name);

   const int Memsize = 65536;
   const char *const shMemName = process_name.c_str();
   const int max = 100;

   {
      //Compare several shared memory vector operations with std::vector
      //Create shared memory
      shared_memory_object::remove(shMemName);
      try{
         ManagedSharedMemory segment(create_only, shMemName, Memsize);

         segment.reserve_named_objects(100);

         //Shared memory allocator must be always be initialized
         //since it has no default constructor
         MyShmVector *shmvector = segment.template construct<MyShmVector>("MyShmVector")
                                 (segment.get_segment_manager());
         MyStdVector *stdvector = new MyStdVector;

         shmvector->resize(100);
         stdvector->resize(100);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         shmvector->resize(200);
         stdvector->resize(200);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         shmvector->resize(0);
         stdvector->resize(0);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         for(int i = 0; i < max; ++i){
            IntType new_int(i);
            shmvector->insert(shmvector->end(), boost::move(new_int));
            stdvector->insert(stdvector->end(), i);
            if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
         }
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         typename MyShmVector::iterator shmit(shmvector->begin());
         typename MyStdVector::iterator stdit(stdvector->begin());
         typename MyShmVector::const_iterator cshmit = shmit;
         (void)cshmit;
         ++shmit; ++stdit;
         shmvector->erase(shmit);
         stdvector->erase(stdit);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         shmvector->erase(shmvector->begin());
         stdvector->erase(stdvector->begin());
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         {
            //Initialize values
            IntType aux_vect[50];
            for(int i = 0; i < 50; ++i){
               IntType new_int(-1);
               //BOOST_STATIC_ASSERT((::boost::move_ipcdetail::is_copy_constructible<boost::interprocess::test::movable_int>::value == false));
               aux_vect[i] = boost::move(new_int);
            }
            int aux_vect2[50];
            for(int i = 0; i < 50; ++i){
               aux_vect2[i] = -1;
            }

            shmvector->insert(shmvector->end()
                              ,::boost::make_move_iterator(&aux_vect[0])
                              ,::boost::make_move_iterator(aux_vect + 50));
            stdvector->insert(stdvector->end(), aux_vect2, aux_vect2 + 50);
            if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

            for(int i = 0, j = static_cast<int>(shmvector->size()); i < j; ++i){
               shmvector->erase(shmvector->begin());
               stdvector->erase(stdvector->begin());
            }
            if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
         }
         {
            IntType aux_vect[50];
            for(int i = 0; i < 50; ++i){
               IntType new_int(-1);
               aux_vect[i] = boost::move(new_int);
            }
            int aux_vect2[50];
            for(int i = 0; i < 50; ++i){
               aux_vect2[i] = -1;
            }
            shmvector->insert(shmvector->begin()
                              ,::boost::make_move_iterator(&aux_vect[0])
                              ,::boost::make_move_iterator(aux_vect + 50));
            stdvector->insert(stdvector->begin(), aux_vect2, aux_vect2 + 50);
            if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
         }

         shmvector->reserve(shmvector->size()*2);
         stdvector->reserve(stdvector->size()*2);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         IntType push_back_this(1);
         shmvector->push_back(boost::move(push_back_this));
         stdvector->push_back(int(1));
         shmvector->push_back(IntType(1));
         stdvector->push_back(int(1));
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         if(!copyable_only(shmvector, stdvector
                        ,ipcdetail::bool_<!ipcdetail::is_same<IntType, test::movable_int>::value>())){
            return 1;
         }

         shmvector->erase(shmvector->begin());
         stdvector->erase(stdvector->begin());
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         for(int i = 0; i < max; ++i){
            IntType insert_this(i);
            shmvector->insert(shmvector->begin(), boost::move(insert_this));
            stdvector->insert(stdvector->begin(), i);
            shmvector->insert(shmvector->begin(), IntType(i));
            stdvector->insert(stdvector->begin(), int(i));
         }
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;

         //Test insertion from list
         {
            std::list<int> l(50, int(1));
            shmvector->insert(shmvector->begin(), l.begin(), l.end());
            stdvector->insert(stdvector->begin(), l.begin(), l.end());
            if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
            shmvector->assign(l.begin(), l.end());
            stdvector->assign(l.begin(), l.end());
            if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
         }
/*
         std::size_t cap = shmvector->capacity();
         shmvector->reserve(cap*2);
         stdvector->reserve(cap*2);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
         shmvector->resize(0);
         stdvector->resize(0);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
         shmvector->resize(cap*2);
         stdvector->resize(cap*2);
         if(!test::CheckEqualContainers(shmvector, stdvector)) return 1;
*/

         delete stdvector;
         segment.template destroy<MyShmVector>("MyShmVector");
         segment.shrink_to_fit_indexes();

         if(!segment.all_memory_deallocated())
            return 1;
      }
      catch(std::exception &ex){
         shared_memory_object::remove(shMemName);
         std::cout << ex.what() << std::endl;
         return 1;
      }
   }
   shared_memory_object::remove(shMemName);
   std::cout << std::endl << "Test OK!" << std::endl;
   return 0;
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
