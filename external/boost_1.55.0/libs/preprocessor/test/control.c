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
# include <boost/preprocessor/arithmetic/add.hpp>
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/control.hpp>
# include <libs/preprocessor/test/test.h>

# define TR(x) 1

BEGIN BOOST_PP_EXPR_IIF(0, TR)(0) == 0 END
BEGIN BOOST_PP_EXPR_IIF(1, TR)(0) == 1 END

BEGIN BOOST_PP_EXPR_IF(3, TR)(0) == 1 END
BEGIN BOOST_PP_EXPR_IF(0, TR)(0) == 0 END

BEGIN BOOST_PP_IIF(0, 1, 0) == 0 END
BEGIN BOOST_PP_IIF(1, 1, 0) == 1 END

BEGIN BOOST_PP_IF(0, 1, 0) == 0 END
BEGIN BOOST_PP_IF(9, 1, 0) == 1 END

# define PRED(d, state) state
# define OP_1(d, state) BOOST_PP_DEC(state)

BEGIN BOOST_PP_WHILE(PRED, OP_1, 50) == 0 END

# define OP_2(d, state) BOOST_PP_DEC(BOOST_PP_ADD(BOOST_PP_WHILE(PRED, OP_1, state), state))
# define OP_3(d, state) BOOST_PP_DEC(BOOST_PP_ADD_D(d, BOOST_PP_WHILE_ ## d(PRED, OP_1, state), state))

BEGIN BOOST_PP_WHILE(PRED, OP_2, 10) == 0 END
BEGIN BOOST_PP_WHILE(PRED, OP_3, 10) == 0 END
