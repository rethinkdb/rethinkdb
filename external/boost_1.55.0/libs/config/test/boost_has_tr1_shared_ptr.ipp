//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TR1_SHARED_PTR
//  TITLE:         std::tr1::shared_ptr
//  DESCRIPTION:   The std lib has a tr1-conforming shrared_ptr.

#include <memory>

namespace boost_has_tr1_shared_ptr{

int test()
{
   int i;
   std::tr1::shared_ptr<int> r(new int());
   (void)r;
   return 0;
}

}
