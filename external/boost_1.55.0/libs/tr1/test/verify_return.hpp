//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TR1_TEST_RESULT_HPP
#define BOOST_TR1_TEST_RESULT_HPP

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/static_assert.hpp>

template <class T1, class T2>
void verify_return_type(T1, T2)
{
   // This is a workaround for the broken type
   // deduction in Borland C++:
   typedef typename boost::remove_cv<T1>::type T1CV;
   typedef typename boost::remove_cv<T2>::type T2CV;
   BOOST_STATIC_ASSERT( (::boost::is_same<T1CV, T2CV>::value));
}


#endif

