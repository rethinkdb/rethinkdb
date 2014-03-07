//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_STDINT_H
//  TITLE:         stdint.h
//  DESCRIPTION:   There are no 1998 C++ Standard headers <stdint.h> 
//                 or <cstdint>, although the 1999 C Standard does
//                 include <stdint.h>.
//                 If <stdint.h> is present, <boost/stdint.h> can make
//                 good use of it, so a flag is supplied (signalling
//                 presence; thus the default is not present, conforming
//                 to the current C++ standard).

# if defined(__hpux) || defined(__FreeBSD__) || defined(__IBMCPP__)
#   include <inttypes.h>
# else
#   include <stdint.h>
# endif

namespace boost_has_stdint_h{

int test()
{
   int8_t i = 0;
#ifndef __QNX__
   // QNX has these under non-standard names, our cstdint.hpp will find them however:
   int_fast8_t j = 0;
   int_least8_t k = 0;
   (void)j;
   (void)k;
#endif
   (void)i;

   return 0;
}

}







