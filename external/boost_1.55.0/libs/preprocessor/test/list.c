# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* Revised by Edward Diener (2011) */
#
# /* See http://www.boost.org for most recent version. */
#
# include <boost/preprocessor/arithmetic/add.hpp>
# include <boost/preprocessor/arithmetic/sub.hpp>
# include <boost/preprocessor/comparison/less.hpp>
# include <boost/preprocessor/list.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/array/elem.hpp>
# include <boost/preprocessor/seq/elem.hpp>
# include <libs/preprocessor/test/test.h>

# define LIST (4, (1, (5, (2, BOOST_PP_NIL))))

# define REVERSAL(d, x, y) BOOST_PP_SUB_D(d, y, x)

BEGIN BOOST_PP_LIST_FOLD_LEFT(BOOST_PP_SUB_D, 22, LIST) == 10 END
BEGIN BOOST_PP_LIST_FOLD_RIGHT(BOOST_PP_ADD_D, 0, LIST) == 12 END
BEGIN BOOST_PP_LIST_FOLD_RIGHT(REVERSAL, 0, LIST) == 4 END

BEGIN BOOST_PP_LIST_CAT(BOOST_PP_LIST_REVERSE(LIST)) == 2514 END

BEGIN BOOST_PP_LIST_CAT(BOOST_PP_LIST_REST_N(2, LIST)) == 52 END
BEGIN BOOST_PP_LIST_CAT(BOOST_PP_LIST_FIRST_N(2, LIST)) == 41 END

BEGIN BOOST_PP_LIST_AT(LIST, 2) == 5 END
BEGIN BOOST_PP_LIST_SIZE(LIST) == 4 END

BEGIN BOOST_PP_LIST_CAT(BOOST_PP_LIST_TRANSFORM(BOOST_PP_ADD_D, 2, LIST)) == 6374 END
BEGIN BOOST_PP_LIST_CAT(BOOST_PP_LIST_APPEND(BOOST_PP_LIST_REST(LIST), LIST)) == 1524152 END

# define F1(r, state, x) + x + state
BEGIN BOOST_PP_LIST_FOR_EACH(F1, 1, LIST) == 16 END

BEGIN BOOST_PP_TUPLE_ELEM(4, 3, BOOST_PP_LIST_TO_TUPLE(LIST)) == 2 END

BEGIN BOOST_PP_LIST_CAT(BOOST_PP_LIST_FILTER(BOOST_PP_LESS_D, 3, LIST)) == 45 END

# define F2(r, x) + BOOST_PP_TUPLE_ELEM(2, 0, x) + 2 - BOOST_PP_TUPLE_ELEM(2, 1, x)
BEGIN BOOST_PP_LIST_FOR_EACH_PRODUCT(F2, 2, ( (1, (0, BOOST_PP_NIL)), (2, (3, BOOST_PP_NIL)) )) == 0 END

# define L1 (0, (x, BOOST_PP_NIL))
# define L2 (a, (1, (b, (2, BOOST_PP_NIL))))
# define L3 (c, (3, (d, BOOST_PP_NIL)))

# define LL (L1, (L2, (L3, BOOST_PP_NIL)))

BEGIN BOOST_PP_LIST_CAT(BOOST_PP_LIST_FOLD_LEFT(BOOST_PP_LIST_APPEND_D, BOOST_PP_NIL, LL)) == 0x0a1b2c3d END

BEGIN BOOST_PP_ARRAY_ELEM(2, BOOST_PP_LIST_TO_ARRAY(LIST)) == 5 END
BEGIN BOOST_PP_SEQ_ELEM(3, BOOST_PP_LIST_TO_SEQ(LIST)) == 2 END
