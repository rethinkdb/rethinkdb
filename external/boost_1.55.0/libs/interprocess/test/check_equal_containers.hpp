//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_CHECK_EQUAL_CONTAINERS_HPP
#define BOOST_INTERPROCESS_TEST_CHECK_EQUAL_CONTAINERS_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <functional>
#include <iostream>
#include <algorithm>
#include <boost/container/detail/pair.hpp>

namespace boost{
namespace interprocess{
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
template<class MyShmCont
        ,class MyStdCont>
bool CheckEqualContainers(MyShmCont *shmcont, MyStdCont *stdcont)
{
   if(shmcont->size() != stdcont->size())
      return false;

   typename MyShmCont::iterator itshm(shmcont->begin()), itshmend(shmcont->end());
   typename MyStdCont::iterator itstd(stdcont->begin());
   typename MyStdCont::size_type dist = (typename MyStdCont::size_type)std::distance(itshm, itshmend);
   if(dist != shmcont->size()){
      return false;
   }
   std::size_t i = 0;
   for(; itshm != itshmend; ++itshm, ++itstd, ++i){
      if(!CheckEqual(*itstd, *itshm))
         return false;
   }
   return true;
}

template<class MyShmCont
        ,class MyStdCont>
bool CheckEqualPairContainers(MyShmCont *shmcont, MyStdCont *stdcont)
{
   if(shmcont->size() != stdcont->size())
      return false;

   typedef typename MyShmCont::key_type      key_type;
   typedef typename MyShmCont::mapped_type   mapped_type;

   typename MyShmCont::iterator itshm(shmcont->begin()), itshmend(shmcont->end());
   typename MyStdCont::iterator itstd(stdcont->begin());
   for(; itshm != itshmend; ++itshm, ++itstd){
      if(itshm->first != key_type(itstd->first))
         return false;

      if(itshm->second != mapped_type(itstd->second))
         return false;
   }
   return true;
}
}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_TEST_CHECK_EQUAL_CONTAINERS_HPP
