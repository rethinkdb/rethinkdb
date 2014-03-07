//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TR1_TYPE_TRAITS
//  TITLE:         std::tr1::type_traits
//  DESCRIPTION:   The std lib has a tr1-conforming type traits library.

#include <type_traits>

namespace boost_has_tr1_type_traits{

using std::tr1::is_void;

int test()
{
   return 0;
}

}
