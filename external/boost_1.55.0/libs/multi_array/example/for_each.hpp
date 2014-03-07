// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

#ifndef FOR_EACH_HPP
#define FOR_EACH_HPP

//
// for_each.hpp - Writing an algorithm to transform each element of
//   a multi_array
//

#include "boost/type.hpp"

template <typename Array, typename Element, typename Functor>
void for_each (const boost::type<Element>& type_dispatch,
               Array A, Functor& xform) {
  for_each(type_dispatch,A.begin(),A.end(),xform);
}

template <typename Element, typename Functor>
void for_each (const boost::type<Element>&,Element& Val, Functor& xform) {
  Val = xform(Val);
}

template <typename Element, typename Iterator, typename Functor>
void for_each (const boost::type<Element>& type_dispatch,
               Iterator begin, Iterator end,
               Functor& xform) {
  while (begin != end) {
    for_each(type_dispatch,*begin,xform);
    ++begin;
  }
}


template <typename Array, typename Functor>
void for_each (Array& A, Functor xform) {
  // Dispatch to the proper function
  for_each(boost::type<typename Array::element>(),A.begin(),A.end(),xform);
}


#endif // FOR_EACH_HPP
