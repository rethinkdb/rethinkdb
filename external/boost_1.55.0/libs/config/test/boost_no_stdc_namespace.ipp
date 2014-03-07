//  (C) Copyright John Maddock 2001.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_STDC_NAMESPACE
//  TITLE:         std:: namespace for C API's
//  DESCRIPTION:   The contents of C++ standard headers for C library
//                 functions (the <c...> headers) have not been placed
//                 in namespace std.  This test is difficult - some libraries
//                 "fake" the std C functions by adding using declarations
//                 to import them into namespace std, unfortunately they don't
//                 necessarily catch all of them...

#include <cstring>
#include <cctype>
#include <ctime>

#undef isspace
#undef isalpha
#undef ispunct

namespace boost_no_stdc_namespace{


int test()
{
   char c = 0;
#ifndef BOOST_NO_CTYPE_FUNCTIONS
   (void)std::isspace(c);
   (void)std::isalpha(c);
   (void)std::ispunct(c);
#endif
   (void)std::strlen(&c);
   (void)std::clock();

   return 0;
}

}

