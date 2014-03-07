////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
////////////////////////////////////////

#ifndef BOOST_CONTAINER_TEST_MAP_TEST_HEADER
#define BOOST_CONTAINER_TEST_MAP_TEST_HEADER

#include <boost/container/detail/config_begin.hpp>
#include "check_equal_containers.hpp"
#include <map>
#include <functional>
#include <utility>
#include "print_container.hpp"
#include <boost/container/detail/utilities.hpp>
#include <boost/container/detail/pair.hpp>
#include <boost/move/iterator.hpp>
#include <boost/move/utility.hpp>
#include <string>

template<class T1, class T2, class T3, class T4>
bool operator ==(std::pair<T1, T2> &p1, std::pair<T1, T2> &p2)
{
   return p1.first == p2.first && p1.second == p2.second;
}

namespace boost{
namespace container {
namespace test{

template<class MyBoostMap
        ,class MyStdMap
        ,class MyBoostMultiMap
        ,class MyStdMultiMap>
int map_test ()
{
   typedef typename MyBoostMap::key_type    IntType;
   typedef container_detail::pair<IntType, IntType>         IntPairType;
   typedef typename MyStdMap::value_type  StdPairType;
   const int max = 100;

   BOOST_TRY{
      MyBoostMap *boostmap = new MyBoostMap;
      MyStdMap *stdmap = new MyStdMap;
      MyBoostMultiMap *boostmultimap = new MyBoostMultiMap;
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

         MyBoostMap *boostmap2 = new MyBoostMap
               ( boost::make_move_iterator(&aux_vect[0])
               , boost::make_move_iterator(aux_vect + 50));
         MyStdMap *stdmap2 = new MyStdMap(aux_vect2, aux_vect2 + 50);
         MyBoostMultiMap *boostmultimap2 = new MyBoostMultiMap
               ( boost::make_move_iterator(&aux_vect3[0])
               , boost::make_move_iterator(aux_vect3 + 50));
         MyStdMultiMap *stdmultimap2 = new MyStdMultiMap(aux_vect2, aux_vect2 + 50);
         if(!CheckEqualContainers(boostmap2, stdmap2)) return 1;
         if(!CheckEqualContainers(boostmultimap2, stdmultimap2)) return 1;

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
         if(!CheckEqualContainers(boostmap2, stdmap2)) return 1;
         if(!CheckEqualContainers(boostmultimap2, stdmultimap2)) return 1;

         MyBoostMap *boostmap3 = new MyBoostMap
               ( ordered_unique_range
               , boost::make_move_iterator(&aux_vect[0])
               , boost::make_move_iterator(aux_vect + 50));
         MyStdMap *stdmap3 = new MyStdMap(aux_vect2, aux_vect2 + 50);
         MyBoostMultiMap *boostmultimap3 = new MyBoostMultiMap
               ( ordered_range
               , boost::make_move_iterator(&aux_vect3[0])
               , boost::make_move_iterator(aux_vect3 + 50));
         MyStdMultiMap *stdmultimap3 = new MyStdMultiMap(aux_vect2, aux_vect2 + 50);

         if(!CheckEqualContainers(boostmap3, stdmap3)){
            std::cout << "Error in construct<MyBoostMap>(MyBoostMap3)" << std::endl;
            return 1;
         }
         if(!CheckEqualContainers(boostmultimap3, stdmultimap3)){
            std::cout << "Error in construct<MyBoostMultiMap>(MyBoostMultiMap3)" << std::endl;
            return 1;
         }

         {
            IntType i0(0);
            boostmap2->erase(i0);
            boostmultimap2->erase(i0);
            stdmap2->erase(0);
            stdmultimap2->erase(0);
         }
         {
            IntType i0(0);
            IntType i1(1);
            (*boostmap2)[::boost::move(i0)] = ::boost::move(i1);
         }
         {
            IntType i1(1);
            (*boostmap2)[IntType(0)] = ::boost::move(i1);
         }
         (*stdmap2)[0] = 1;
         if(!CheckEqualContainers(boostmap2, stdmap2)) return 1;

         delete boostmap2;
         delete boostmultimap2;
         delete stdmap2;
         delete stdmultimap2;
         delete boostmap3;
         delete boostmultimap3;
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
            boostmap->insert(boost::move(aux_vect[i]));
            stdmap->insert(StdPairType(i, i));
            boostmultimap->insert(boost::move(aux_vect3[i]));
            stdmultimap->insert(StdPairType(i, i));
         }

         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;

         typename MyBoostMap::iterator it = boostmap->begin();
         typename MyBoostMap::const_iterator cit = it;
         (void)cit;

         boostmap->erase(boostmap->begin());
         stdmap->erase(stdmap->begin());
         boostmultimap->erase(boostmultimap->begin());
         stdmultimap->erase(stdmultimap->begin());
         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;

         boostmap->erase(boostmap->begin());
         stdmap->erase(stdmap->begin());
         boostmultimap->erase(boostmultimap->begin());
         stdmultimap->erase(stdmultimap->begin());
         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;

         //Swapping test
         MyBoostMap tmpboostemap2;
         MyStdMap tmpstdmap2;
         MyBoostMultiMap tmpboostemultimap2;
         MyStdMultiMap tmpstdmultimap2;
         boostmap->swap(tmpboostemap2);
         stdmap->swap(tmpstdmap2);
         boostmultimap->swap(tmpboostemultimap2);
         stdmultimap->swap(tmpstdmultimap2);
         boostmap->swap(tmpboostemap2);
         stdmap->swap(tmpstdmap2);
         boostmultimap->swap(tmpboostemultimap2);
         stdmultimap->swap(tmpstdmultimap2);
         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;
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

         boostmap->insert(boost::make_move_iterator(&aux_vect[0]), boost::make_move_iterator(aux_vect + 50));
         boostmultimap->insert(boost::make_move_iterator(&aux_vect3[0]), boost::make_move_iterator(aux_vect3 + 50));
         for(std::size_t i = 0; i != 50; ++i){
            StdPairType stdpairtype(-1, -1);
            stdmap->insert(stdpairtype);
            stdmultimap->insert(stdpairtype);
         }
         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;

         for(int i = 0, j = static_cast<int>(boostmap->size()); i < j; ++i){
            boostmap->erase(IntType(i));
            stdmap->erase(i);
            boostmultimap->erase(IntType(i));
            stdmultimap->erase(i);
         }
         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;
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

         boostmap->insert(boost::make_move_iterator(&aux_vect[0]), boost::make_move_iterator(aux_vect + 50));
         boostmap->insert(boost::make_move_iterator(&aux_vect3[0]), boost::make_move_iterator(aux_vect3 + 50));
         boostmultimap->insert(boost::make_move_iterator(&aux_vect4[0]), boost::make_move_iterator(aux_vect4 + 50));
         boostmultimap->insert(boost::make_move_iterator(&aux_vect5[0]), boost::make_move_iterator(aux_vect5 + 50));

         for(std::size_t i = 0; i != 50; ++i){
            StdPairType stdpairtype(-1, -1);
            stdmap->insert(stdpairtype);
            stdmultimap->insert(stdpairtype);
            stdmap->insert(stdpairtype);
            stdmultimap->insert(stdpairtype);
         }
         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;

         boostmap->erase(boostmap->begin()->first);
         stdmap->erase(stdmap->begin()->first);
         boostmultimap->erase(boostmultimap->begin()->first);
         stdmultimap->erase(stdmultimap->begin()->first);
         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;
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
            boostmap->insert(boost::move(aux_vect[i]));
            stdmap->insert(StdPairType(i, i));
            boostmultimap->insert(boost::move(aux_vect3[i]));
            stdmultimap->insert(StdPairType(i, i));
         }

         if(!CheckEqualPairContainers(boostmap, stdmap)) return 1;
         if(!CheckEqualPairContainers(boostmultimap, stdmultimap)) return 1;

         for(int i = 0; i < max; ++i){
            IntPairType intpair;
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            boostmap->insert(boostmap->begin(), boost::move(intpair));
            stdmap->insert(stdmap->begin(), StdPairType(i, i));
            //PrintContainers(boostmap, stdmap);
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            boostmultimap->insert(boostmultimap->begin(), boost::move(intpair));
            stdmultimap->insert(stdmultimap->begin(), StdPairType(i, i));
            //PrintContainers(boostmultimap, stdmultimap);
            if(!CheckEqualPairContainers(boostmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(boostmultimap, stdmultimap))
               return 1;
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            boostmap->insert(boostmap->end(), boost::move(intpair));
            stdmap->insert(stdmap->end(), StdPairType(i, i));
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            boostmultimap->insert(boostmultimap->end(), boost::move(intpair));
            stdmultimap->insert(stdmultimap->end(), StdPairType(i, i));
            if(!CheckEqualPairContainers(boostmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(boostmultimap, stdmultimap))
               return 1;
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            boostmap->insert(boostmap->lower_bound(IntType(i)), boost::move(intpair));
            stdmap->insert(stdmap->lower_bound(i), StdPairType(i, i));
            //PrintContainers(boostmap, stdmap);
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            {
               IntType i1(i);
               boostmultimap->insert(boostmultimap->lower_bound(boost::move(i1)), boost::move(intpair));
               stdmultimap->insert(stdmultimap->lower_bound(i), StdPairType(i, i));
            }

            //PrintContainers(boostmultimap, stdmultimap);
            if(!CheckEqualPairContainers(boostmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(boostmultimap, stdmultimap))
               return 1;
            {  //Check equal_range
               std::pair<typename MyBoostMultiMap::iterator, typename MyBoostMultiMap::iterator> bret =
                  boostmultimap->equal_range(boostmultimap->begin()->first);

               std::pair<typename MyStdMultiMap::iterator, typename MyStdMultiMap::iterator>   sret =
                  stdmultimap->equal_range(stdmultimap->begin()->first);
        
               if( std::distance(bret.first, bret.second) !=
                   std::distance(sret.first, sret.second) ){
                  return 1;
               }
            }
            {
               IntType i1(i);
               boostmap->insert(boostmap->upper_bound(boost::move(i1)), boost::move(intpair));
               stdmap->insert(stdmap->upper_bound(i), StdPairType(i, i));
            }
            //PrintContainers(boostmap, stdmap);
            {
               IntType i1(i);
               IntType i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            {
               IntType i1(i);
               boostmultimap->insert(boostmultimap->upper_bound(boost::move(i1)), boost::move(intpair));
               stdmultimap->insert(stdmultimap->upper_bound(i), StdPairType(i, i));
            }
            //PrintContainers(boostmultimap, stdmultimap);
            if(!CheckEqualPairContainers(boostmap, stdmap))
               return 1;
            if(!CheckEqualPairContainers(boostmultimap, stdmultimap))
               return 1;
         }

         //Compare count with std containers
         for(int i = 0; i < max; ++i){
            if(boostmap->count(IntType(i)) != stdmap->count(i)){
               return -1;
            }

            if(boostmultimap->count(IntType(i)) != stdmultimap->count(i)){
               return -1;
            }
         }

         //Now do count exercise
         boostmap->erase(boostmap->begin(), boostmap->end());
         boostmultimap->erase(boostmultimap->begin(), boostmultimap->end());
         boostmap->clear();
         boostmultimap->clear();

         for(int j = 0; j < 3; ++j)
         for(int i = 0; i < 100; ++i){
            IntPairType intpair;
            {
            IntType i1(i), i2(i);
            new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            boostmap->insert(boost::move(intpair));
            {
               IntType i1(i), i2(i);
               new(&intpair)IntPairType(boost::move(i1), boost::move(i2));
            }
            boostmultimap->insert(boost::move(intpair));
            if(boostmap->count(IntType(i)) != typename MyBoostMultiMap::size_type(1))
               return 1;
            if(boostmultimap->count(IntType(i)) != typename MyBoostMultiMap::size_type(j+1))
               return 1;
         }
      }

      delete boostmap;
      delete stdmap;
      delete boostmultimap;
      delete stdmultimap;
   }
   BOOST_CATCH(...){
      BOOST_RETHROW;
   }
   BOOST_CATCH_END
   return 0;
}

template<class MyBoostMap
        ,class MyStdMap
        ,class MyBoostMultiMap
        ,class MyStdMultiMap>
int map_test_copyable ()
{
   typedef typename MyBoostMap::key_type    IntType;
   typedef container_detail::pair<IntType, IntType>         IntPairType;
   typedef typename MyStdMap::value_type  StdPairType;

   const int max = 100;

   BOOST_TRY{
   MyBoostMap *boostmap = new MyBoostMap;
   MyStdMap *stdmap = new MyStdMap;
   MyBoostMultiMap *boostmultimap = new MyBoostMultiMap;
   MyStdMultiMap *stdmultimap = new MyStdMultiMap;

   int i;
   for(i = 0; i < max; ++i){
      {
      IntType i1(i), i2(i);
      IntPairType intpair1(boost::move(i1), boost::move(i2));
      boostmap->insert(boost::move(intpair1));
      stdmap->insert(StdPairType(i, i));
      }
      {
      IntType i1(i), i2(i);
      IntPairType intpair2(boost::move(i1), boost::move(i2));
      boostmultimap->insert(boost::move(intpair2));
      stdmultimap->insert(StdPairType(i, i));
      }
   }
   if(!CheckEqualContainers(boostmap, stdmap)) return 1;
   if(!CheckEqualContainers(boostmultimap, stdmultimap)) return 1;

      {
         //Now, test copy constructor
         MyBoostMap boostmapcopy(*boostmap);
         MyStdMap stdmapcopy(*stdmap);
         MyBoostMultiMap boostmmapcopy(*boostmultimap);
         MyStdMultiMap stdmmapcopy(*stdmultimap);

         if(!CheckEqualContainers(&boostmapcopy, &stdmapcopy))
            return 1;
         if(!CheckEqualContainers(&boostmmapcopy, &stdmmapcopy))
            return 1;

         //And now assignment
         boostmapcopy  = *boostmap;
         stdmapcopy  = *stdmap;
         boostmmapcopy = *boostmultimap;
         stdmmapcopy = *stdmultimap;
        
         if(!CheckEqualContainers(&boostmapcopy, &stdmapcopy))
            return 1;
         if(!CheckEqualContainers(&boostmmapcopy, &stdmmapcopy))
            return 1;
         delete boostmap;
         delete boostmultimap;
         delete stdmap;
         delete stdmultimap;
      }
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

#endif   //#ifndef BOOST_CONTAINER_TEST_MAP_TEST_HEADER
