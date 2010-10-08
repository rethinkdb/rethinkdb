dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl We check two things: where is the memory include file, and in what
dnl namespace does shared_ptr reside.
dnl We include AC_COMPILE_IFELSE for all the combinations we've seen in the
dnl wild:
dnl 
dnl  GCC 4.3: namespace: std::  #include <memory>
dnl  GCC 4.2: namespace: tr1::  #include <tr1/memory>
dnl  GCC 4.2: namespace: boost::  #include <boost/shared_ptr.hpp>
dnl
dnl We define one of HAVE_HAVE_TR1_SHARED_PTR or HAVE_BOOST_SHARED_PTR
dnl depending on location, and SHARED_PTR_NAMESPACE to be the namespace in
dnl which shared_ptr is defined.
dnl 

AC_DEFUN([PANDORA_SHARED_PTR],[
  AC_REQUIRE([PANDORA_CHECK_CXX_STANDARD])
  AC_LANG_PUSH(C++)
  save_CXXFLAGS="${CXXFLAGS}"
  CXXFLAGS="${CXX_STANDARD} ${CXXFLAGS}"
  AC_CHECK_HEADERS(memory tr1/memory boost/shared_ptr.hpp)
  AC_CACHE_CHECK([the location of shared_ptr header file],
    [ac_cv_shared_ptr_h],[
      for namespace in std tr1 std::tr1 boost
      do
        AC_COMPILE_IFELSE(
          [AC_LANG_PROGRAM([[
#if defined(HAVE_MEMORY)
# include <memory>
#endif
#if defined(HAVE_TR1_MEMORY)
# include <tr1/memory>
#endif
#if defined(HAVE_BOOST_SHARED_PTR_HPP)
# include <boost/shared_ptr.hpp>
#endif
#include <string>

using $namespace::shared_ptr;
using namespace std;
            ]],[[
shared_ptr<string> test_ptr(new string("test string"));
            ]])],
            [
              ac_cv_shared_ptr_namespace="${namespace}"
              break
            ],[ac_cv_shared_ptr_namespace=missing])
       done
  ])
  AC_DEFINE_UNQUOTED([SHARED_PTR_NAMESPACE],
                     ${ac_cv_shared_ptr_namespace},
                     [The namespace in which SHARED_PTR can be found])
  CXXFLAGS="${save_CXXFLAGS}"
  AC_LANG_POP()
])
