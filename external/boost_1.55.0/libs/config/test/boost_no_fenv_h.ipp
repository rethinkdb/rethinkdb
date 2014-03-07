//  (C) Copyright John Maddock 2001.
//  (C) Copyright Bryce Lelbach 2010. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_FENV_H
//  TITLE:         fenv.h
//  DESCRIPTION:   There is no standard <fenv.h> available. If <fenv.h> is
//                 available, <boost/detail/fenv.hpp> should be included
//                 instead of directly including <fenv.h>.

#include <fenv.h>

namespace boost_no_fenv_h {

int test()
{
  /// C++0x required typedefs
  typedef ::fenv_t has_fenv_t;
  typedef ::fexcept_t has_fexcept_t;

  /// C++0x required macros
  #if !defined(FE_DIVBYZERO)
    #error platform does not define FE_DIVBYZERO
  #endif
  
  #if !defined(FE_INEXACT)
    #error platform does not define FE_INEXACT
  #endif

  #if !defined(FE_ALL_EXCEPT)
    #error platform does not define FE_ALL_EXCEPT
  #endif

   int i;
   fexcept_t fe;
   fenv_t env;
  
   i = feclearexcept(FE_ALL_EXCEPT);
   i += fetestexcept(FE_ALL_EXCEPT); // All flags should be zero
   i += fegetexceptflag(&fe, FE_ALL_EXCEPT);
   i += fesetexceptflag(&fe, FE_ALL_EXCEPT);
   i += feraiseexcept(0);
   i += fesetround(fegetround());
   i += fegetenv(&env);
   i += fesetenv(&env);
   i += feholdexcept(&env);
   if(i)
      i += feupdateenv(&env);

   return i;
}

}

