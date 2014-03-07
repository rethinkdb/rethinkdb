//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <tuple>
#include <functional>
#include <utility>
#else
#include <boost/tr1/tuple.hpp>
#include <boost/tr1/functional.hpp>
#include <boost/tr1/utility.hpp>
#endif

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include "verify_return.hpp"

int main()
{
   std::tr1::tuple<int> t1a;
   std::tr1::tuple<int> t1b(0);
   std::tr1::tuple<int> t1c(t1b);
   t1a = t1c;
   std::tr1::tuple<int> t1d(std::tr1::tuple<short>(0));
   t1a = std::tr1::tuple<short>(0);

   std::tr1::tuple<int, long> t2a;
   std::tr1::tuple<int, long> t2b(0, 0);
   std::tr1::tuple<int, long> t2c(t2b);
   t2a = t2c;
   std::tr1::tuple<int, long> t2d(std::tr1::tuple<short, int>(0, 0));
   t2a = std::tr1::tuple<short, int>(0, 0);
   //std::tr1::tuple<int, long> t2e(std::make_pair(0, 0L));
   //t2e = std::make_pair(0, 0L);
  
   // check implementation limits:
   std::tr1::tuple<int, long, float, int, int, int, int, int, int> t10(0, 0, 0, 0, 0, 0, 0, 0, 0);

   // make_tuple:
   verify_return_type(std::tr1::make_tuple(0, 0, 0L), std::tr1::tuple<int, int, long>());
   int i = 0;
   std::tr1::tuple<int&, int, long> t3a(std::tr1::make_tuple(std::tr1::ref(i), 0, 0L));
   verify_return_type(std::tr1::make_tuple(std::tr1::ref(i), 0, 0L), t3a);
   std::tr1::tuple<const int&, int, long> t3b(std::tr1::make_tuple(std::tr1::cref(i), 0, 0L));
   verify_return_type(std::tr1::make_tuple(std::tr1::cref(i), 0, 0L), t3b);
   long j = 0;
   std::tr1::tuple<int&, long&> tt(std::tr1::tie(i,j));

   BOOST_STATIC_ASSERT((::std::tr1::tuple_size<std::tr1::tuple<int, long> >::value == 2));
   BOOST_STATIC_ASSERT((::std::tr1::tuple_size<std::tr1::tuple<int, long, float, int, int, int, int, int, int> >::value == 9));

   BOOST_STATIC_ASSERT((::boost::is_same< ::std::tr1::tuple_element<0, std::tr1::tuple<int, long> >::type, int>::value));
   BOOST_STATIC_ASSERT((::boost::is_same< ::std::tr1::tuple_element<1, std::tr1::tuple<int, long> >::type, long>::value));
   BOOST_STATIC_ASSERT((::boost::is_same< ::std::tr1::tuple_element<0, std::tr1::tuple<int&, long> >::type, int&>::value));
   BOOST_STATIC_ASSERT((::boost::is_same< ::std::tr1::tuple_element<0, std::tr1::tuple<const int&, long> >::type, const int&>::value));

   // get:
   verify_return_type(&::std::tr1::get<0>(t1a), static_cast<int*>(0));
   verify_return_type(&::std::tr1::get<1>(t2d), static_cast<long*>(0));
   verify_return_type(&::std::tr1::get<0>(t3a), static_cast<int*>(0));
   verify_return_type(&::std::tr1::get<0>(t3b), static_cast<const int*>(0));
   const std::tr1::tuple<int>& cr1 = t1a;
   verify_return_type(&::std::tr1::get<0>(cr1), static_cast<const int*>(0));
   const std::tr1::tuple<int&, int, long>& cr2 = t3a;
   verify_return_type(&::std::tr1::get<0>(cr2), static_cast<int*>(0));
   const std::tr1::tuple<const int&, int, long>& cr3 = t3b;
   // comparison:
   verify_return_type(cr2 == cr3, false);
   verify_return_type(cr2 != cr3, false);
   verify_return_type(cr2 < cr3, false);
   verify_return_type(cr2 > cr3, false);
   verify_return_type(cr2 <= cr3, false);
   verify_return_type(cr2 >= cr3, false);

   // pair interface:
   BOOST_STATIC_ASSERT((::std::tr1::tuple_size<std::pair<int, long> >::value == 2));
   BOOST_STATIC_ASSERT((::std::tr1::tuple_size<std::pair<int, float> >::value == 2));
   BOOST_STATIC_ASSERT((::boost::is_same< ::std::tr1::tuple_element<0, std::pair<int, long> >::type, int>::value));
   BOOST_STATIC_ASSERT((::boost::is_same< ::std::tr1::tuple_element<1, std::pair<int, long> >::type, long>::value));

   std::pair<int, long> p1;
   const std::pair<int, long>& p2 = p1;
   verify_return_type(&std::tr1::get<0>(p1), static_cast<int*>(0));
   verify_return_type(&std::tr1::get<1>(p1), static_cast<long*>(0));
   verify_return_type(&std::tr1::get<0>(p2), static_cast<const int*>(0));
   verify_return_type(&std::tr1::get<1>(p2), static_cast<const long*>(0));

   return 0;
}

