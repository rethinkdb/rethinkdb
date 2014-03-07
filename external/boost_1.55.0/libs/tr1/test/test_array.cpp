//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <array>
#else
#include <boost/tr1/array.hpp>
#endif

#include <string>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include "verify_return.hpp"

template <class T>
void check_array(T& a)
{
   typedef typename T::reference reference;
   typedef typename T::const_reference const_reference;
   typedef typename T::iterator iterator;
   typedef typename T::const_iterator const_iterator;
   typedef typename T::size_type size_type;
   typedef typename T::difference_type difference_type;
   typedef typename T::value_type value_type;
   typedef typename T::reverse_iterator reverse_iterator;
   typedef typename T::const_reverse_iterator const_reverse_iterator;

   BOOST_STATIC_ASSERT((::boost::is_same<value_type&, reference>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<value_type const&, const_reference>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::size_t, size_type>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::ptrdiff_t, difference_type>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::reverse_iterator<iterator>, reverse_iterator>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<std::reverse_iterator<const_iterator>, const_reverse_iterator>::value));

   const T& ca = a;
   const T& ca2 = a;
   verify_return_type(a.begin(), iterator());
   verify_return_type(ca.begin(), const_iterator());
   verify_return_type(a.end(), iterator());
   verify_return_type(ca.end(), const_iterator());
   verify_return_type(a.rbegin(), reverse_iterator());
   verify_return_type(ca.rbegin(), const_reverse_iterator());
   verify_return_type(a.rend(), reverse_iterator());
   verify_return_type(ca.rend(), const_reverse_iterator());

   verify_return_type(ca.size(), size_type(0));
   verify_return_type(ca.max_size(), size_type(0));
   verify_return_type(ca.empty(), false);

   verify_return_type(&a[0], static_cast<value_type*>(0));
   verify_return_type(&ca[0], static_cast<const value_type*>(0));
   verify_return_type(&a.at(0), static_cast<value_type*>(0));
   verify_return_type(&ca.at(0), static_cast<const value_type*>(0));
   verify_return_type(&a.front(), static_cast<value_type*>(0));
   verify_return_type(&ca.front(), static_cast<const value_type*>(0));
   verify_return_type(&a.back(), static_cast<value_type*>(0));
   verify_return_type(&ca.back(), static_cast<const value_type*>(0));
   //verify_return_type(a.data(), static_cast<value_type*>(0));
   verify_return_type(ca.data(), static_cast<const value_type*>(0));

   // swap:
   std::tr1::swap(a, a);
   verify_return_type(ca == ca2, false);
   verify_return_type(ca != ca2, false);
   verify_return_type(ca < ca2, false);
   verify_return_type(ca > ca2, false);
   verify_return_type(ca <= ca2, false);
   verify_return_type(ca >= ca2, false);

   BOOST_STATIC_ASSERT((::boost::is_same< typename std::tr1::tuple_element<0,T>::type, value_type>::value));
   verify_return_type(&std::tr1::get<0>(a), static_cast<value_type*>(0));
   verify_return_type(&std::tr1::get<0>(ca), static_cast<const value_type*>(0));
}

int main()
{
   std::tr1::array<int,2> a1 = {};
   check_array(a1);
   BOOST_STATIC_ASSERT((std::tr1::tuple_size<std::tr1::array<int,2> >::value == 2));
 
   std::tr1::array<std::string,4> a2 = { "abc", "def", "", "y", };
   check_array(a2);
   BOOST_STATIC_ASSERT((std::tr1::tuple_size<std::tr1::array<std::string,4> >::value == 4));
   return 0;
}

