//
//  adapted from bind_stdcall_test.cpp - test for bind.hpp + __stdcall (free functions)
//   The purpose of this simple test is to determine if a function can be
//   called from Python with the various existing calling conventions 
//
//  Copyright (c) 2001 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#if !defined(TEST_INCLUDE_RECURSION)

#define TEST_INCLUDE_RECURSION

//------------------------------------------------------------------------------
// this section is the main body of the test extension module

#define BOOST_PYTHON_ENABLE_CDECL
#define BOOST_PYTHON_ENABLE_STDCALL
#define BOOST_PYTHON_ENABLE_FASTCALL
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/python.hpp>
using namespace boost::python;

// first define test functions for every calling convention

#define TEST_DECLARE_FUNCTIONS

#define TESTED_CALLING_CONVENTION __cdecl
#include "calling_conventions.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __stdcall
#include "calling_conventions.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __fastcall
#include "calling_conventions.cpp"
#undef TESTED_CALLING_CONVENTION

#undef TEST_DECLARE_FUNCTIONS 

// then create a module wrapping the defined functions for every calling convention

BOOST_PYTHON_MODULE( calling_conventions_ext )
{

#define TEST_WRAP_FUNCTIONS

#define TESTED_CALLING_CONVENTION __cdecl
#include "calling_conventions.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __stdcall
#include "calling_conventions.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __fastcall
#include "calling_conventions.cpp"
#undef TESTED_CALLING_CONVENTION

#undef TEST_WRAP_FUNCTIONS

}

#else // !defined(TEST_INCLUDE_RECURSION)

//------------------------------------------------------------------------------
// this section defines the functions to be wrapped

# if defined(TEST_DECLARE_FUNCTIONS)

#  if !defined(TESTED_CALLING_CONVENTION)
#   error "One calling convention must be defined"
#  endif // !defined(TESTED_CALLING_CONVENTION)

namespace BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION) { 

  long TESTED_CALLING_CONVENTION f_0()
  {
      return 17041L;
  }
  
  long TESTED_CALLING_CONVENTION f_1(long a)
  {
      return a;
  }
  
  long TESTED_CALLING_CONVENTION f_2(long a, long b)
  {
      return a + 10 * b;
  }
  
  long TESTED_CALLING_CONVENTION f_3(long a, long b, long c)
  {
      return a + 10 * b + 100 * c;
  }
  
  long TESTED_CALLING_CONVENTION f_4(long a, long b, long c, long d)
  {
      return a + 10 * b + 100 * c + 1000 * d;
  }
  
  long TESTED_CALLING_CONVENTION f_5(long a, long b, long c, long d, long e)
  {
      return a + 10 * b + 100 * c + 1000 * d + 10000 * e;
  }
  
  long TESTED_CALLING_CONVENTION f_6(long a, long b, long c, long d, long e, long f)
  {
      return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
  }
  
  long TESTED_CALLING_CONVENTION f_7(long a, long b, long c, long d, long e, long f, long g)
  {
      return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
  }
  
  long TESTED_CALLING_CONVENTION f_8(long a, long b, long c, long d, long e, long f, long g, long h)
  {
      return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
  }
  
  long TESTED_CALLING_CONVENTION f_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
  {
      return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h + 100000000 * i;
  }
  
} // namespace test##TESTED_CALLING_CONVENTION

# endif // defined(TEST_DECLARE_FUNCTIONS)

//------------------------------------------------------------------------------
// this section wraps the functions

# if defined(TEST_WRAP_FUNCTIONS)

#  if !defined(TESTED_CALLING_CONVENTION)
#   error "One calling convention must be defined"
#  endif // !defined(TESTED_CALLING_CONVENTION)

    def("f_0" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_0);
    def("f_1" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_1);
    def("f_2" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_2);
    def("f_3" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_3);
    def("f_4" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_4);
    def("f_5" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_5);
    def("f_6" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_6);
    def("f_7" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_7);
    def("f_8" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_8);
    def("f_9" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION), &BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::f_9);

# endif // defined(TEST_WRAP_FUNCTIONS)

#endif // !defined(TEST_INCLUDE_RECURSION)
