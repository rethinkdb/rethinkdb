# We check two things: where the include file is for unordered_map, and
# what namespace unordered_map lives in within that include file.  We
# include AC_COMPILE_IFELSE for all the combinations we've seen in the
# wild.  We define HAVE_UNORDERED_MAP and HAVE_UNORDERED_SET if we have
# them, UNORDERED_MAP_H and UNORDERED_SET_H to their location and
# UNORDERED_NAMESPACE to be the namespace unordered_map is defined in.

AC_DEFUN([PANDORA_CXX_STL_UNORDERED],[
  save_CXXFLAGS="${CXXFLAGS}"
  CXXFLAGS="${AM_CXXFLAGS} ${CXXFLAGS}"
  AC_LANG_PUSH(C++)
  AC_CACHE_CHECK([for STL unordered_map],
    [pandora_cv_stl_unordered],[
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <unordered_map>]],
                         [[std::unordered_map<int, int> t]])],
        [pandora_cv_stl_unordered="yes"],
        [pandora_cv_stl_unordered="no"])])

  AS_IF([test "x${pandora_cv_stl_unordered}" != "xyes"],[
    AC_CACHE_CHECK([for tr1 unordered_map],
      [pandora_cv_tr1_unordered],[
        AC_COMPILE_IFELSE(
          [AC_LANG_PROGRAM([[
/* We put in this define because of a YACC symbol clash in Drizzle.
   Seriously... I cannot believe the GCC guys defined a piece of the internals
   of this named IF - and I can't believe that YACC generates a token define
   called IF. Really?
*/
#define IF 21;
#include <tr1/unordered_map>
            ]],[[
std::tr1::unordered_map<int, int> t
          ]])],
          [pandora_cv_tr1_unordered="yes"],
          [pandora_cv_tr1_unordered="no"])])
  ])

  AS_IF([test "x${pandora_cv_stl_unordered}" != "xyes" -a "x${pandora_cv_tr1_unordered}" != "xyes"],[
    AC_CACHE_CHECK([for boost unordered_map],
      [pandora_cv_boost_unordered],[
        AC_COMPILE_IFELSE(
          [AC_LANG_PROGRAM([[#include <boost/unordered_map.hpp>]],
                           [[boost::unordered_map<int, int> t]])],
          [pandora_cv_boost_unordered="yes"],
          [pandora_cv_boost_unordered="no"])])
  ])

  CXXFLAGS="${save_CXXFLAGS}"
  AC_LANG_POP()

  AS_IF([test "x${pandora_cv_stl_unordered}" = "xyes"],[
    AC_DEFINE(HAVE_STD_UNORDERED_MAP, 1,
              [if the compiler has std::unordered_map])
    AC_DEFINE(HAVE_STD_UNORDERED_SET, 1,
              [if the compiler has std::unordered_set])
    pandora_has_unordered=yes
  ])
  AS_IF([test "x${pandora_cv_tr1_unordered}" = "xyes"],[
    AC_DEFINE(HAVE_TR1_UNORDERED_MAP, 1,
              [if the compiler has std::tr1::unordered_map])
    AC_DEFINE(HAVE_TR1_UNORDERED_SET, 1,
              [if the compiler has std::tr1::unordered_set])
    pandora_has_unordered=yes
  ])
  AS_IF([test "x${pandora_cv_boost_unordered}" = "xyes"],[
    AC_DEFINE(HAVE_BOOST_UNORDERED_MAP, 1,
              [if the compiler has boost::unordered_map])
    AC_DEFINE(HAVE_BOOST_UNORDERED_SET, 1,
              [if the compiler has boost::unordered_set])
    pandora_has_unordered=yes
  ])
    
  AS_IF([test "x${pandora_has_unordered}" != "xyes"],[
    AC_MSG_WARN([could not find an STL unordered_map])
  ])
])

AC_DEFUN([PANDORA_HAVE_CXX_UNORDERED],[
  AC_REQUIRE([PANDORA_CXX_STL_UNORDERED])
])

AC_DEFUN([PANDORA_REQUIRE_CXX_UNORDERED],[
  AC_REQUIRE([PANDORA_HAVE_CXX_UNORDERED])
  AS_IF([test "x${pandora_has_unordered}" != "xyes"],[
    AC_MSG_ERROR([An STL compliant unordered_map is required for ${PACKAGE}.
    Implementations can be found in Recent versions of gcc and in boost])
  ])
])
