# /* Copyright (C) 2001
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_LIBS_PREPROCESSOR_REGRESSION_TEST_H
# define BOOST_LIBS_PREPROCESSOR_REGRESSION_TEST_H
#
# include <boost/preprocessor/cat.hpp>
#
# define BEGIN typedef int BOOST_PP_CAT(test_, __LINE__)[((
# define END )==1) ? 1 : -1];

#if defined(__cplusplus)
#include <cstdio>
#if !defined(_STLP_MSVC) || _STLP_MSVC >= 1300
namespace std { }
using namespace std;
#endif
#else
#include <stdio.h>
#endif

int main(void) {
    printf("pass " __TIME__);
    return 0;
}

# endif
