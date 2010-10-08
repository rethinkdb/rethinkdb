dnl Copyright (C) 2010 Monty Taylor
dnl This file is free software; Monty Taylor
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_BOOST_PROGRAM_OPTIONS],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for boost::program_options
  dnl --------------------------------------------------------------------

  AC_LANG_PUSH(C++)
  AC_LIB_HAVE_LINKFLAGS(boost_program_options-mt,,[
    #include <boost/program_options.hpp>
  ],[
    boost::program_options::options_description d;
    d.add_options()("a","some option");
  ])
  AS_IF([test "x${ac_cv_libboost_program_options_mt}" = "xno"],[
    AC_LIB_HAVE_LINKFLAGS(boost_program_options,,[
      #include <boost/program_options.hpp>
    ],[
      boost::program_options::options_description d;
      d.add_options()("a","some option");
    ])
  ])
  AC_LANG_POP()
  
  AM_CONDITIONAL(HAVE_BOOST_PROGRAM_OPTIONS,
    [test "x${ac_cv_libboost_program_options}" = "xyes" -o "x${ac_cv_libboost_program_options_mt}" = "xyes"])
  BOOST_LIBS="${BOOST_LIBS} ${LTLIBBOOST_PROGRAM_OPTIONS} ${LTLIBBOOST_PROGRAM_OPTIONS_MT}"
  AC_SUBST(BOOST_LIBS) 
])

AC_DEFUN([PANDORA_HAVE_BOOST_PROGRAM_OPTIONS],[
  PANDORA_HAVE_BOOST($1)
  _PANDORA_SEARCH_BOOST_PROGRAM_OPTIONS($1)
])

AC_DEFUN([PANDORA_REQUIRE_BOOST_PROGRAM_OPTIONS],[
  PANDORA_REQUIRE_BOOST($1)
  _PANDORA_SEARCH_BOOST_PROGRAM_OPTIONS($1)
  AS_IF([test "x${ac_cv_libboost_program_options}" = "xno" -a "x${ac_cv_libboost_program_options_mt}" = "xno"],
      AC_MSG_ERROR([boost::program_options is required for ${PACKAGE}]))
])

