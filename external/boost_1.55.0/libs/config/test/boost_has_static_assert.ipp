//  Copyright (C) 2007 Douglas Gregor
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_STATIC_ASSERT
//  TITLE:         static assertions
//  DESCRIPTION:   The compiler supports C++0x static assertions

namespace boost_has_static_assert {

int test()
{
   static_assert(true, "OK");
   return 0;
}

}
