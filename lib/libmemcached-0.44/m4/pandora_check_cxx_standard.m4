dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([PANDORA_CHECK_CXX_STANDARD],[
  AC_REQUIRE([AC_CXX_COMPILE_STDCXX_0X])
  AS_IF([test "$GCC" = "yes"],
        [AS_IF([test "$ac_cv_cxx_compile_cxx0x_native" = "yes"],[],
               [AS_IF([test "$ac_cv_cxx_compile_cxx0x_gxx" = "yes"],
                      [CXX_STANDARD="-std=gnu++0x"],
                      [CXX_STANDARD="-std=gnu++98"])
               ])
        ])
  AM_CXXFLAGS="${CXX_STANDARD} ${AM_CXXFLAGS}"
  
  save_CXXFLAGS="${CXXFLAGS}"
  CXXFLAGS="${CXXFLAGS} ${CXX_STANDARD}"
  AC_CXX_HEADER_STDCXX_98
  CXXFLAGS="${save_CXXFLAGS}"

  AC_SUBST([CXX_STANDARD])
])
