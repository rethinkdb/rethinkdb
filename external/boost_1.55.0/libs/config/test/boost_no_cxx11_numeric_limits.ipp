//  (C) Copyright Vicente J. Botet Escriba 2010. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_NUMERIC_LIMITS
//  TITLE:         static function lowest() in numeric_limits class <limits>
//  DESCRIPTION:   static function numeric_limits<T>::lowest() are not available for use.

#include <limits>

namespace boost_no_cxx11_numeric_limits{

int f()
{
    // this is never called, it just has to compile:
    return std::numeric_limits<int>::lowest();
}

int test()
{
   return 0;
}

}




