//  common_type.hpp  ---------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_EX_TYPE_TRAITS_EXT_DECLVAL__HPP
#define BOOST_EX_TYPE_TRAITS_EXT_DECLVAL__HPP

#include <boost/config.hpp>

//----------------------------------------------------------------------------//

#include "add_rvalue_reference.hpp"

//----------------------------------------------------------------------------//
//                                                                            //
//                           C++03 implementation of                          //
//                          Written by Vicente J. Botet Escriba               //
//~ 20.3.4 Function template declval [declval]
//~ 1 The library provides the function template declval to simplify the definition of expressions which occur as
//~ unevaluated operands.
//~ 2 Remarks: If this function is used, the program is ill-formed.
//~ 3 Remarks: The template parameter T of declval may be an incomplete type.
//~ [ Example:

//~ template <class To, class From>
//~ decltype(static_cast<To>(declval<From>())) convert(From&&);

//~ declares a function template convert which only participats in overloading if the type From can be
//~ explicitly converted to type To. For another example see class template common_type (20.7.6.6). -end
//~ example ]
//                                                                            //
//----------------------------------------------------------------------------//

namespace boost_ex {

    template <typename T>
    typename add_rvalue_reference<T>::type declval(); //noexcept; // as unevaluated operand

}  // namespace boost

#endif  // BOOST_EX_TYPE_TRAITS_EXT_DECLVAL__HPP
