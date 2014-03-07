////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_MAP_TEST_HEADER
#define BOOST_INTERPROCESS_TEST_MAP_TEST_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include "check_equal_containers.hpp"
#include <map>
#include <functional>
#include <utility>
#include "print_container.hpp"
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/containers/pair.hpp>
#include <string>
#include "get_process_id_name.hpp"

template<class T1, class T2, class T3, class T4>
bool operator ==(std::pair<T1, T2> &p1, std::pair<T1, T2> &p2)
{
   return p1.first == p2.first && p1.second == p2.second;
}

namespace boost{
namespace interprocess{
namespace test{

template<class ManagedSharedMemory
        ,class MyShmMap
        ,class MyStdMap
        ,class MyShmMultiMap
        ,class MyStdMultiMap>
int map_test ()
{
   typedef typename MyShmMap::key_type    IntType;
   typedef boost::interprocess::pair<IntType, IntType>         IntPairType;
   typedef typename MyStdMap::value_type  StdPairType;
   const int memsize = 65536;
   const char *const shMemName = test::get_process_id_name();
   const int max = 100;

   try{
      //Create shared memory
      shared_memory_object::remove(shMemName);
      ManagedSharedMemory segment(create_only, shMemName, memsize);

      segment.reserve_named_objects(100);

      //Shared memory allocator must be always be initialized
      //since it has no default constructor
      MyShmMap *shmmap =
         segment.template construct<MyShmMap>("MyShmMap")
            (std::less<IntType>(), segment.get_segment_manager());

      MyStdMap *stdmap = new MyStdMap;

      MyShmMultiMap *shmmultimap =
         segment.template construct<MyShmMultiMap>("MyShmMultiMap")
            (std::less<IntType>(), segment.get_segment_manager());

      MyStdMultiMap *stdmultimap = new MyStdMultiMap;

      //Test construction from a range
      {
         //This is really nasty, but we have no other simple choice
         IntPairType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(i/2);
            IntType i2(i/2);
            new(&aux_vect[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         typedef typename MyStdMap::value_type StdValueType;
         typedef typename MyStdMap::key_type StdKeyType;
         typedef typename MyStdMap::mapped_type StdMappedType;
         StdValueType aux_vect2[50];
         for(int i = 0; i < 50; ++i){
            new(&aux_vect2[i])StdValueType(StdKeyType(i/2), StdMappedType(i/2));
         }

         IntPairType aux_vect3[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(i/2);
            IntType i2(i/2);
            new(&aux_vect3[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         MyShmMap *shmmap2 =
            segment.template construct<MyShmMap>("MyShmMap2")
               ( ::boost::make_move_iterator(&aux_vect[0])
               , ::boost::make_move_iterator(aux_vect + 50)
               , std::less<IntType>(), segment.get_segment_manager());

         MyStdMap *stdmap2 = new MyStdMap(aux_vect2, aux_vect2 + 50);

         MyShmMultiMap *shmmultimap2 =
            segment.template construct<MyShmMultiMap>("MyShmMultiMap2")
               ( ::boost::make_move_iterator(&aux_vect3[0])
               , ::boost::make_move_iterator(aux_vect3 + 50)
               , std::less<IntType>(), segment.get_segment_manager());

         MyStdMultiMap *stdmultimap2 = new MyStdMultiMap(aux_vect2, aux_vect2 + 50);
         if(!CheckEqualContainers(shmmap2, stdmap2)) return 1;
         if(!CheckEqualContainers(shmmultimap2, stdmultimap2)) return 1;

         //ordered range insertion
         //This is really nasty, but we have no other simple choice
         for(int i = 0; i < 50; ++i){
            IntType i1(i);
            IntType i2(i);
            new(&aux_vect[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         for(int i = 0; i < 50; ++i){
            new(&aux_vect2[i])StdValueType(StdKeyType(i), StdMappedType(i));
         }

         for(int i = 0; i < 50; ++i){
            IntType i1(i);
            IntType i2(i);
            new(&aux_vect3[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         MyShmMap *shmmap3 =
            segment.template construct<MyShmMap>("MyShmMap3")
               ( ordered_unique_range
               , ::boost::make_move_iterator(&aux_vect[0])
               , ::boost::make_move_iterator(aux_vect + 50)
               , std::less<IntType>(), segment.get_segment_manager());

         MyStdMap *stdmap3 = new MyStdMap(aux_vect2, aux_vect2 + 50);

         MyShmMultiMap *shmmultimap3 =
            segment.template construct<MyShmMultiMap>("MyShmMultiMap3")
               ( ordered_range
               , ::boost::make_move_iterator(&aux_vect3[0])
               , ::boost::make_move_iterator(aux_vect3 + 50)
               , std::less<IntType>(), segment.get_segment_manager());

         MyStdMultiMap *stdmultimap3 = new MyStdMultiMap(aux_vect2, aux_vect2 + 50);

         if(!CheckEqualContainers(shmmap3, stdmap3)){
            std::cout << "Error in construct<MyShmMap>(MyShmMap3)" << std::endl;
            return 1;
         }
         if(!CheckEqualContainers(shmmultimap3, stdmultimap3)){
            std::cout << "Error in construct<MyShmMultiMap>(MyShmMultiMap3)" << std::endl;
            return 1;
         }

         segment.destroy_ptr(shmmap2);
         segment.destroy_ptr(shmmultimap2);
         delete stdmap2;
         delete stdmultimap2;
         segment.destroy_ptr(shmmap3);
         segment.destroy_ptr(shmmultimap3);
         delete stdmap3;
         delete stdmultimap3;
      }
      {
         //This is really nasty, but we have no other simple choice
         IntPairType aux_vect[max];
         for(int i = 0; i < max; ++i){
            IntType i1(i);
            IntType i2(i);
            new(&aux_vect[i])IntPairType(boost::move(i1), boost::move(i2));
         }
         IntPairType aux_vect3[max];
         for(int i = 0; i < max; ++i){
            IntType i1(i);
            IntType i2(i);
            new(&aux_vect3[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         for(int i = 0; i < max; ++i){
            shmmap->insert(boost::move(aux_vect[i]));
            stdmap->insert(StdPairType(i, i));
            shmmultimap->insert(boost::move(aux_vect3[i]));
            stdmultimap->insert(StdPairType(i, i));
         }

         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         typename MyShmMap::iterator it;
         typename MyShmMap::const_iterator cit = it;
         (void)cit;

         shmmap->erase(shmmap->begin()++);
         stdmap->erase(stdmap->begin()++);
         shmmultimap->erase(shmmultimap->begin()++);
         stdmultimap->erase(stdmultimap->begin()++);
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         shmmap->erase(shmmap->begin());
         stdmap->erase(stdmap->begin());
         shmmultimap->erase(shmmultimap->begin());
         stdmultimap->erase(stdmultimap->begin());
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         //Swapping test
         std::less<IntType> lessfunc;
         MyShmMap tmpshmemap2 (lessfunc, segment.get_segment_manager());
         MyStdMap tmpstdmap2;
         MyShmMultiMap tmpshmemultimap2(lessfunc, segment.get_segment_manager());
         MyStdMultiMap tmpstdmultimap2;
         shmmap->swap(tmpshmemap2);
         stdmap->swap(tmpstdmap2);
         shmmultimap->swap(tmpshmemultimap2);
         stdmultimap->swap(tmpstdmultimap2);
         shmmap->swap(tmpshmemap2);
         stdmap->swap(tmpstdmap2);
         shmmultimap->swap(tmpshmemultimap2);
         stdmultimap->swap(tmpstdmultimap2);
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;
      }
      //Insertion from other container
      //Initialize values
      {
         //This is really nasty, but we have no other simple choice
         IntPairType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(-1);
            IntType i2(-1);
            new(&aux_vect[i])IntPairType(boost::move(i1), boost::move(i2));
         }
         IntPairType aux_vect3[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(-1);
            IntType i2(-1);
            new(&aux_vect3[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         shmmap->insert(::boost::make_move_iterator(&aux_vect[0]), ::boost::make_move_iterator(aux_vect + 50));
         shmmultimap->insert(::boost::make_move_iterator(&aux_vect3[0]), ::boost::make_move_iterator(aux_vect3 + 50));
         for(std::size_t i = 0; i != 50; ++i){
            StdPairType stdpairtype(-1, -1);
            stdmap->insert(stdpairtype);
            stdmultimap->insert(stdpairtype);
         }
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         for(int i = 0, j = static_cast<int>(shmmap->size()); i < j; ++i){
            shmmap->erase(IntType(i));
            stdmap->erase(i);
            shmmultimap->erase(IntType(i));
            stdmultimap->erase(i);
         }
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;
      }
      {
         IntPairType aux_vect[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(-1);
            IntType i2(-1);
            new(&aux_vect[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         IntPairType aux_vect3[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(-1);
            IntType i2(-1);
            new(&aux_vect3[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         IntPairType aux_vect4[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(-1);
            IntType i2(-1);
            new(&aux_vect4[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         IntPairType aux_vect5[50];
         for(int i = 0; i < 50; ++i){
            IntType i1(-1);
            IntType i2(-1);
            new(&aux_vect5[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         shmmap->insert(::boost::make_move_iterator(&aux_vect[0]), ::boost::make_move_iterator(aux_vect + 50));
         shmmap->insert(::boost::make_move_iterator(&aux_vect3[0]), ::boost::make_move_iterator(aux_vect3 + 50));
         shmmultimap->insert(::boost::make_move_iterator(&aux_vect4[0]), ::boost::make_move_iterator(aux_vect4 + 50));
         shmmultimap->insert(::boost::make_move_iterator(&aux_vect5[0]), ::boost::make_move_iterator(aux_vect5 + 50));

         for(std::size_t i = 0; i != 50; ++i){
            StdPairType stdpairtype(-1, -1);
            stdmap->insert(stdpairtype);
            stdmultimap->insert(stdpairtype);
            stdmap->insert(stdpairtype);
            stdmultimap->insert(stdpairtype);
         }
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         shmmap->erase(shmmap->begin()->first);
         stdmap->erase(stdmap->begin()->first);
         shmmultimap->erase(shmmultimap->begin()->first);
         stdmultimap->erase(stdmultimap->begin()->first);
         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;
      }

      {
         //This is really nasty, but we have no other simple choice
         IntPairType aux_vect[max];
         for(int i = 0; i < max; ++i){
            IntType i1(i);
            IntType i2(i);
            new(&aux_vect[i])IntPairType(boost::move(i1), boost::move(i2));
         }
         IntPairType aux_vect3[max];
         for(int i = 0; i < max; ++i){
            IntType i1(i);
            IntType i2(i);
            new(&aux_vect3[i])IntPairType(boost::move(i1), boost::move(i2));
         }

         for(int i = 0; i < max; ++i){
            shmmap->insert(boost::move(aux_vect[i]));
            stdmap->insert(StdPairType(i, i));
            shmmultimap->insert(boost::move(aux_vect3[i]));
            stdmultimap->insert(StdPairType(i, i));
         }

         if(!CheckEqualPairContainers(shmmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(shmmultimap, stdmultimap)) return 1;

         for(int i = 0; i < max; ++i){
            IntPairType intpair;
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            shmmap->insert(shmmap->begin(), boost::move(intpair));
            stdmap->insert(stdmap->begin(), StdPairType(i, i));
            //PrintContainers(shmmap, stdmap);
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            shmmultimap->insert(shmmultimap->begin(), boost::move(intpair));
            stdmultimap->insert(stdmultimap->begin(), StdPairType(i, i));
            //PrintContainers(shmmultimap, stdmultimap);
            if(!CheckEqualPairContainers(shmmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
               return 1;
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            shmmap->insert(shmmap->end(), boost::move(intpair));
            stdmap->insert(stdmap->end(), StdPairType(i, i));
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            shmmultimap->insert(shmmultimap->end(), boost::move(intpair));
            stdmultimap->insert(stdmultimap->end(), StdPairType(i, i));
            if(!CheckEqualPairContainers(shmmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
               return 1;
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            shmmap->insert(shmmap->lower_bound(IntType(i)), boost::move(intpair));
            stdmap->insert(stdmap->lower_bound(i), StdPairType(i, i));
            //PrintContainers(shmmap, stdmap);
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            {
               IntType i1(i);
               shmmultimap->insert(shmmultimap->lower_bound(boost::move(i1)), boost::move(intpair));
               stdmultimap->insert(stdmultimap->lower_bound(i), StdPairType(i, i));
            }

            //PrintContainers(shmmultimap, stdmultimap);
            if(!CheckEqualPairContainers(shmmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
               return 1;
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            {
               IntType i1(i);
               shmmap->insert(shmmap->upper_bound(boost::move(i1)), boost::move(intpair));
               stdmap->insert(stdmap->upper_bound(i), StdPairType(i, i));
            }
            //PrintContainers(shmmap, stdmap);
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            {
               IntType i1(i);
               shmmultimap->insert(shmmultimap->upper_bound(boost::move(i1)), boost::move(intpair));
               stdmultimap->insert(stdmultimap->upper_bound(i), StdPairType(i, i));
            }
            //PrintContainers(shmmultimap, stdmultimap);
            if(!CheckEqualPairContainers(shmmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(shmmultimap, stdmultimap))
               return 1;
         }

         //Compare count with std containers
         for(int i = 0; i < max; ++i){
            if(shmmap->count(IntType(i)) != stdmap->count(i)){
               return -1;
            }

            if(shmmultimap->count(IntType(i)) != stdmultimap->count(i)){
               return -1;
            }
         }

         //Now do count exercise
         shmmap->erase(shmmap->begin(), shmmap->end());
         shmmultimap->erase(shmmultimap->begin(), shmmultimap->end());
         shmmap->clear();
         shmmultimap->clear();

         for(int j = 0; j < 3; ++j)
         for(int i = 0; i < 100; ++i){
            IntPairType intpair;
            {
            IntType i1(i), i2(i);
            new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            shmmap->insert(boost::move(intpair));
            {
               IntType i1(i), i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            shmmultimap->insert(boost::move(intpair));
            if(shmmap->count(IntType(i)) != typename MyShmMultiMap::size_type(1))
               return 1;
            if(shmmultimap->count(IntType(i)) != typename MyShmMultiMap::size_type(j+1))
               return 1;
         }
      }

      segment.template destroy<MyShmMap>("MyShmMap");
      delete stdmap;
      segment.destroy_ptr(shmmultimap);
      delete stdmultimap;

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

template<class ManagedSharedMemory
        ,class MyShmMap
        ,class MyStdMap
        ,class MyShmMultiMap
        ,class MyStdMultiMap>
int map_test_copyable ()
{
   typedef typename MyShmMap::key_type    IntType;
   typedef boost::interprocess::pair<IntType, IntType>         IntPairType;
   typedef typename MyStdMap::value_type  StdPairType;

   const int memsize = 65536;
   const char *const shMemName = test::get_process_id_name();
   const int max = 100;

   try{
   //Create shared memory
   shared_memory_object::remove(shMemName);
   ManagedSharedMemory segment(create_only, shMemName, memsize);

   segment.reserve_named_objects(100);

   //Shared memory allocator must be always be initialized
   //since it has no default constructor
   MyShmMap *shmmap =
      segment.template construct<MyShmMap>("MyShmMap")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdMap *stdmap = new MyStdMap;

   MyShmMultiMap *shmmultimap =
      segment.template construct<MyShmMultiMap>("MyShmMultiMap")
         (std::less<IntType>(), segment.get_segment_manager());

   MyStdMultiMap *stdmultimap = new MyStdMultiMap;

   int i;
   for(i = 0; i < max; ++i){
      {
      IntType i1(i), i2(i);
      IntPairType intpair1(boost::move(i1), boost::move(i2));
      shmmap->insert(boost::move(intpair1));
      stdmap->insert(StdPairType(i, i));
      }
      {
      IntType i1(i), i2(i);
      IntPairType intpair2(boost::move(i1), boost::move(i2));
      shmmultimap->insert(boost::move(intpair2));
      stdmultimap->insert(StdPairType(i, i));
      }
   }
   if(!CheckEqualContainers(shmmap, stdmap)) return 1;
   if(!CheckEqualContainers(shmmultimap, stdmultimap)) return 1;

      {
         //Now, test copy constructor
         MyShmMap shmmapcopy(*shmmap);
         MyStdMap stdmapcopy(*stdmap);
         MyShmMultiMap shmmmapcopy(*shmmultimap);
         MyStdMultiMap stdmmapcopy(*stdmultimap);

         if(!CheckEqualContainers(&shmmapcopy, &stdmapcopy))
            return 1;
         if(!CheckEqualContainers(&shmmmapcopy, &stdmmapcopy))
            return 1;

         //And now assignment
         shmmapcopy  = *shmmap;
         stdmapcopy  = *stdmap;
         shmmmapcopy = *shmmultimap;
         stdmmapcopy = *stdmultimap;

         if(!CheckEqualContainers(&shmmapcopy, &stdmapcopy))
            return 1;
         if(!CheckEqualContainers(&shmmmapcopy, &stdmmapcopy))
            return 1;
         segment.destroy_ptr(shmmap);
         segment.destroy_ptr(shmmultimap);
      }
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

#endif   //#ifndef BOOST_INTERPROCESS_TEST_MAP_TEST_HEADER
