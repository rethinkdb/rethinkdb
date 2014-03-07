//
//  adapted from bind_stdcall_mf_test.cpp - test for bind.hpp + __stdcall (free functions)
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
#include "calling_conventions_mf.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __stdcall
#include "calling_conventions_mf.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __fastcall
#include "calling_conventions_mf.cpp"
#undef TESTED_CALLING_CONVENTION

#undef TEST_DECLARE_FUNCTIONS 

// then create a module wrapping the defined functions for every calling convention

BOOST_PYTHON_MODULE( calling_conventions_mf_ext )
{

#define TEST_WRAP_FUNCTIONS

#define TESTED_CALLING_CONVENTION __cdecl
#include "calling_conventions_mf.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __stdcall
#include "calling_conventions_mf.cpp"
#undef TESTED_CALLING_CONVENTION

#define TESTED_CALLING_CONVENTION __fastcall
#include "calling_conventions_mf.cpp"
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

struct X
{
    mutable unsigned int hash;

    X(): hash(0) {}

    void TESTED_CALLING_CONVENTION f0() { f1(17); }
    void TESTED_CALLING_CONVENTION g0() const { g1(17); }

    void TESTED_CALLING_CONVENTION f1(int a1) { hash = (hash * 17041 + a1) % 32768; }
    void TESTED_CALLING_CONVENTION g1(int a1) const { hash = (hash * 17041 + a1 * 2) % 32768; }

    void TESTED_CALLING_CONVENTION f2(int a1, int a2) { f1(a1); f1(a2); }
    void TESTED_CALLING_CONVENTION g2(int a1, int a2) const { g1(a1); g1(a2); }

    void TESTED_CALLING_CONVENTION f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); }
    void TESTED_CALLING_CONVENTION g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); }

    void TESTED_CALLING_CONVENTION f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); }
    void TESTED_CALLING_CONVENTION g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); }

    void TESTED_CALLING_CONVENTION f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); }
    void TESTED_CALLING_CONVENTION g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); }

    void TESTED_CALLING_CONVENTION f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); }
    void TESTED_CALLING_CONVENTION g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); }

    void TESTED_CALLING_CONVENTION f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); }
    void TESTED_CALLING_CONVENTION g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); }

    void TESTED_CALLING_CONVENTION f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); }
    void TESTED_CALLING_CONVENTION g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); }
};

} // namespace BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)

# endif // defined(TEST_DECLARE_FUNCTIONS)

//------------------------------------------------------------------------------
// this section wraps the functions

# if defined(TEST_WRAP_FUNCTIONS)

#  if !defined(TESTED_CALLING_CONVENTION)
#   error "One calling convention must be defined"
#  endif // !defined(TESTED_CALLING_CONVENTION)

{
  
  typedef BOOST_PP_CAT(test, TESTED_CALLING_CONVENTION)::X X;

  class_<X>("X" BOOST_PP_STRINGIZE(TESTED_CALLING_CONVENTION))
    .def("f0", &X::f0)
    .def("g0", &X::g0)
    .def("f1", &X::f1)
    .def("g1", &X::g1)
    .def("f2", &X::f2)
    .def("g2", &X::g2)
    .def("f3", &X::f3)
    .def("g3", &X::g3)
    .def("f4", &X::f4)
    .def("g4", &X::g4)
    .def("f5", &X::f5)
    .def("g5", &X::g5)
    .def("f6", &X::f6)
    .def("g6", &X::g6)
    .def("f7", &X::f7)
    .def("g7", &X::g7)
    .def("f8", &X::f8)
    .def("g8", &X::g8)
    .def_readonly("hash", &X::hash)
    ;

}

# endif // defined(TEST_WRAP_FUNCTIONS)

#endif // !defined(TEST_INCLUDE_RECURSION)
