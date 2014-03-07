//  (C) Copyright John Maddock 2003. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.

#include "link_test.hpp"

int main()
{
#ifndef BOOST_NO_CXX11_EXTERN_TEMPLATE
   test_free_proc<int>(0);
   tester<int>::test();
#endif
   return check_options(dyn_link, dyn_rtl, has_threads, debug, stl_debug) ? 0 : -1;
}


