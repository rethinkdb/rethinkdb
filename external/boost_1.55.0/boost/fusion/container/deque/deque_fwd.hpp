/*=============================================================================
    Copyright (c) 2005-2012 Joel de Guzman
    Copyright (c) 2005-2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_DEQUE_FORWARD_02092007_0749)
#define FUSION_DEQUE_FORWARD_02092007_0749

#include <boost/config.hpp>

///////////////////////////////////////////////////////////////////////////////
// With no decltype and variadics, we will use the C++03 version
///////////////////////////////////////////////////////////////////////////////
#if (defined(BOOST_NO_CXX11_DECLTYPE)             \
  || defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)   \
  || defined(BOOST_NO_CXX11_RVALUE_REFERENCES))
# include <boost/fusion/container/deque/detail/cpp03/deque_fwd.hpp>
#else
# if !defined(BOOST_FUSION_HAS_VARIADIC_DEQUE)
#   define BOOST_FUSION_HAS_VARIADIC_DEQUE
# endif

///////////////////////////////////////////////////////////////////////////////
// C++11 interface
///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace fusion
{
    template <typename ...T>
    struct deque;
}}

#endif
#endif
