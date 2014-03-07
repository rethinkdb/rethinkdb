//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_NL_TYPES_H
//  TITLE:         <nl_types.h>
//  DESCRIPTION:   The platform has an <nl_types.h>.

#include <nl_types.h>


namespace boost_has_nl_types_h{

int test()
{
   nl_catd cat = catopen("foo", 0);
   if(cat >= 0) catclose(cat);
   return 0;
}

}





