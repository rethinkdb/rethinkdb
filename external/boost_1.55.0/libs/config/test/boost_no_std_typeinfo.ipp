//  (C) Copyright Peter Dimov 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_STD_TYPEINFO
//  TITLE:         type_info not in namespace std
//  DESCRIPTION:   The <typeinfo> header declares type_info in the global namespace instead of std

#include <typeinfo>

namespace boost_no_std_typeinfo
{
void quiet_warning(const std::type_info*){}

int test()
{
   std::type_info * p = 0;
   quiet_warning(p);
   return 0;
}

}

