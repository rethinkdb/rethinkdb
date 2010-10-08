# ===========================================================================
#        http://autoconf-archive.cryp.to/ac_cxx_header_stdcxx_98.html
# ===========================================================================
#
# SYNOPSIS
#
#   AC_CXX_HEADER_STDCXX_98
#
# DESCRIPTION
#
#   Check for complete library coverage of the C++1998/2003 standard.
#
# LICENSE
#
#   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AC_CXX_HEADER_STDCXX_98], [
  AC_CACHE_CHECK(for ISO C++ 98 include files,
  ac_cv_cxx_stdcxx_98,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([
    #include <cassert>
    #include <cctype>
    #include <cerrno>
    #include <cfloat>
    #include <ciso646>
    #include <climits>
    #include <clocale>
    #include <cmath>
    #include <csetjmp>
    #include <csignal>
    #include <cstdarg>
    #include <cstddef>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <ctime>

    #include <algorithm>
    #include <bitset>
    #include <complex>
    #include <deque>
    #include <exception>
    #include <fstream>
    #include <functional>
    #include <iomanip>
    #include <ios>
    #include <iosfwd>
    #include <iostream>
    #include <istream>
    #include <iterator>
    #include <limits>
    #include <list>
    #include <locale>
    #include <map>
    #include <memory>
    #include <new>
    #include <numeric>
    #include <ostream>
    #include <queue>
    #include <set>
    #include <sstream>
    #include <stack>
    #include <stdexcept>
    #include <streambuf>
    #include <string>
    #include <typeinfo>
    #include <utility>
    #include <valarray>
    #include <vector>
  ],,
  ac_cv_cxx_stdcxx_98=yes, ac_cv_cxx_stdcxx_98=no)
  AC_LANG_RESTORE
  ])
  if test "$ac_cv_cxx_stdcxx_98" = yes; then
    AC_DEFINE(STDCXX_98_HEADERS,,[Define if ISO C++ 1998 header files are present. ])
  fi
])
