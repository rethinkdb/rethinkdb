# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# include <boost/preprocessor/arithmetic.hpp>
# include <boost/preprocessor/config/limits.hpp>
# include <libs/preprocessor/test/test.h>

/* addition */

BEGIN BOOST_PP_ADD(2, 3) == 5 END

BEGIN BOOST_PP_ADD(BOOST_PP_ADD(2, 2), 2) == 6 END
BEGIN BOOST_PP_ADD(2, BOOST_PP_ADD(1, 4)) == 7 END
BEGIN BOOST_PP_ADD(BOOST_PP_ADD(2, 2), BOOST_PP_ADD(2, 2)) == 8 END

/* subtraction */

BEGIN BOOST_PP_SUB(11, 0) == 11 END
BEGIN BOOST_PP_SUB(12, 1) == 11 END
BEGIN BOOST_PP_SUB(3, 4) == 0 END

BEGIN BOOST_PP_SUB(5, BOOST_PP_SUB(3, 2)) == 4 END
BEGIN BOOST_PP_SUB(BOOST_PP_SUB(10, 5), 2) == 3 END
BEGIN BOOST_PP_SUB(BOOST_PP_SUB(7, 3), BOOST_PP_SUB(10, 8)) == 2 END

/* multiplication */

BEGIN BOOST_PP_MUL(0, 1) == 0 END
BEGIN BOOST_PP_MUL(1, 0) == 0 END
BEGIN BOOST_PP_MUL(1, 1) == 1 END
BEGIN BOOST_PP_MUL(4, 3) == 12 END

BEGIN BOOST_PP_MUL(BOOST_PP_MUL(2, 2), 2) == 8 END
BEGIN BOOST_PP_MUL(2, BOOST_PP_MUL(2, 2)) == 8 END
BEGIN BOOST_PP_MUL(BOOST_PP_MUL(2, 2), BOOST_PP_MUL(2, 2)) == 16 END

/* division */

BEGIN BOOST_PP_DIV(2, 1) == 2 END
BEGIN BOOST_PP_DIV(0, 5) == 0 END
BEGIN BOOST_PP_DIV(7, 3) == 2 END

BEGIN BOOST_PP_DIV(BOOST_PP_DIV(4, 2), 2) == 1 END
BEGIN BOOST_PP_DIV(10, BOOST_PP_DIV(10, 2)) == 2 END
BEGIN BOOST_PP_DIV(BOOST_PP_DIV(10, 2), BOOST_PP_DIV(4, 2)) == 2 END

/* modulus */

BEGIN BOOST_PP_MOD(5, 5) == 0 END
BEGIN BOOST_PP_MOD(9, 5) == 4 END
BEGIN BOOST_PP_MOD(7, 4) == 3 END

BEGIN BOOST_PP_MOD(BOOST_PP_MOD(5, 3), 3) == 2 END
BEGIN BOOST_PP_MOD(5, BOOST_PP_MOD(4, 3)) == 0 END
