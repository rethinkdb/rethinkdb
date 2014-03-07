//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_TEST_CHECK_EQUAL_CONTAINER_HPP
#define BOOST_CONTAINER_TEST_CHECK_EQUAL_CONTAINER_HPP

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/pair.hpp>
#include <boost/container/detail/mpl.hpp>
#include <functional>
#include <iostream>
#include <algorithm>

namespace boost{
namespace container {
namespace test{

template< class T1, class T2>
bool CheckEqual( const T1 &t1, const T2 &t2
               , typename boost::container::container_detail::enable_if_c
                  <!boost::container::container_detail::is_pair<T1>::value && 
                   !boost::container::container_detail::is_pair<T2>::value
                  >::type* = 0)
{  return t1 == t2;  }

template< class Pair1, class Pair2>
bool CheckEqual( const Pair1 &pair1, const Pair2 &pair2
               , typename boost::container::container_detail::enable_if_c
                  <boost::container::container_detail::is_pair<Pair1>::value && 
                   boost::container::container_detail::is_pair<Pair2>::value
                  >::type* = 0)
{
   return CheckEqual(pair1.first, pair2.first) && CheckEqual(pair1.second, pair2.second);
}

//Function to check if both containers are equal
template<class MyBoostCont
        ,class MyStdCont>
bool CheckEqualContainers(const MyBoostCont *boostcont, const MyStdCont *stdcont)
{
   if(boostcont->size() != stdcont->size())
      return false;

   typename MyBoostCont::const_iterator itboost(boostcont->begin()), itboostend(boostcont->end());
   typename MyStdCont::const_iterator itstd(stdcont->begin());
   typename MyStdCont::size_type dist = (typename MyStdCont::size_type)std::distance(itboost, itboostend);
   if(dist != boostcont->size()){
      return false;
   }
   std::size_t i = 0;
   for(; itboost != itboostend; ++itboost, ++itstd, ++i){
      if(!CheckEqual(*itstd, *itboost))
         return false;
   }
   return true;
}

template<class MyBoostCont
        ,class MyStdCont>
bool CheckEqualPairContainers(const MyBoostCont *boostcont, const MyStdCont *stdcont)
{
   if(boostcont->size() != stdcont->size())
      return false;

   typedef typename MyBoostCont::key_type      key_type;
   typedef typename MyBoostCont::mapped_type   mapped_type;

   typename MyBoostCont::const_iterator itboost(boostcont->begin()), itboostend(boostcont->end());
   typename MyStdCont::const_iterator itstd(stdcont->begin());
   for(; itboost != itboostend; ++itboost, ++itstd){
      if(itboost->first != key_type(itstd->first))
         return false;

      if(itboost->second != mapped_type(itstd->second))
         return false;
   }
   return true;
}
}  //namespace test{
}  //namespace container {
}  //namespace boost{

#include <boost/container/detail/config_end.hpp>

#endif //#ifndef BOOST_CONTAINER_TEST_CHECK_EQUAL_CONTAINER_HPP
