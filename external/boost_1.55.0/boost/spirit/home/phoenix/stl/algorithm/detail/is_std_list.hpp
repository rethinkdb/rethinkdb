// Copyright 2005 Daniel Wallin.
// Copyright 2005 Joel de Guzman.
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler
///////////////////////////////////////////////////////////////////////////////
//
// is_std_list.hpp
//
/////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) && _MSC_VER >= 1000
# pragma once
#endif

#ifndef IS_STD_LIST_EN_16_12_2004
#define IS_STD_LIST_EN_16_12_2004

#include <boost/mpl/bool.hpp>
#include <boost/detail/container_fwd.hpp>

namespace boost
{
    template<class T>
    struct is_std_list
        : boost::mpl::false_
    {};

    template<
        class Ty
      , class Alloc
    >
    struct is_std_list< ::std::list<Ty,Alloc> >
        : boost::mpl::true_
    {};
}

#endif
