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
# include <boost/preprocessor/array.hpp>
# include <libs/preprocessor/test/test.h>
# include <boost/preprocessor/list/at.hpp>
# include <boost/preprocessor/seq/elem.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# if BOOST_PP_VARIADICS
# include <boost/preprocessor/variadic/size.hpp>
# include <boost/preprocessor/variadic/elem.hpp>
# endif

# define ARRAY (3, (0, 1, 2))
# define ARRAY_LARGE (33, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32))
# define ARRAY_VERY_LARGE (64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63))

// element access

BEGIN BOOST_PP_ARRAY_ELEM(1, ARRAY) == 1 END
BEGIN BOOST_PP_ARRAY_ELEM(2, (5, (0, 1, 2, 3, 4))) == 2 END
BEGIN BOOST_PP_ARRAY_ELEM(28, ARRAY_LARGE) == 28 END
BEGIN BOOST_PP_ARRAY_ELEM(17, (33, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32))) == 17 END
BEGIN BOOST_PP_ARRAY_ELEM(42, ARRAY_VERY_LARGE) == 42 END
BEGIN BOOST_PP_ARRAY_ELEM(62, (64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63))) == 62 END

// size

BEGIN BOOST_PP_ARRAY_SIZE(ARRAY) == 3 END
BEGIN BOOST_PP_ARRAY_SIZE((5, (0, 1, 2, 3, 4))) == 5 END
BEGIN BOOST_PP_ARRAY_SIZE(ARRAY_LARGE) == 33 END
BEGIN BOOST_PP_ARRAY_SIZE((33, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32))) == 33 END
BEGIN BOOST_PP_ARRAY_SIZE(ARRAY_VERY_LARGE) == 64 END
BEGIN BOOST_PP_ARRAY_SIZE((64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63))) == 64 END

// enum

# if BOOST_PP_VARIADICS

BEGIN BOOST_PP_VARIADIC_ELEM(2,BOOST_PP_ARRAY_ENUM(ARRAY)) == 2 END
BEGIN BOOST_PP_VARIADIC_ELEM(3,BOOST_PP_ARRAY_ENUM((5, (0, 1, 2, 3, 4)))) == 3 END
BEGIN BOOST_PP_VARIADIC_ELEM(31,BOOST_PP_ARRAY_ENUM(ARRAY_LARGE)) == 31 END
BEGIN BOOST_PP_VARIADIC_ELEM(13,BOOST_PP_ARRAY_ENUM((33, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)))) == 13 END
BEGIN BOOST_PP_VARIADIC_ELEM(39,BOOST_PP_ARRAY_ENUM(ARRAY_VERY_LARGE)) == 39 END
BEGIN BOOST_PP_VARIADIC_ELEM(24,BOOST_PP_ARRAY_ENUM((64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63)))) == 24 END
BEGIN BOOST_PP_VARIADIC_SIZE(BOOST_PP_ARRAY_ENUM((5, (0, 1, 2, 3, 4)))) == 5 END
BEGIN BOOST_PP_VARIADIC_SIZE(BOOST_PP_ARRAY_ENUM(ARRAY_LARGE)) == 33 END
BEGIN BOOST_PP_VARIADIC_SIZE(BOOST_PP_ARRAY_ENUM(ARRAY_VERY_LARGE)) == 64 END
BEGIN BOOST_PP_VARIADIC_SIZE(BOOST_PP_ARRAY_ENUM((64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63)))) == 64 END

# endif

// to_list

BEGIN BOOST_PP_LIST_AT(BOOST_PP_ARRAY_TO_LIST(ARRAY), 1) == 1 END
BEGIN BOOST_PP_LIST_AT(BOOST_PP_ARRAY_TO_LIST((5, (0, 1, 2, 3, 4))), 4) == 4 END
BEGIN BOOST_PP_LIST_AT(BOOST_PP_ARRAY_TO_LIST((33, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32))), 26) == 26 END
BEGIN BOOST_PP_LIST_AT(BOOST_PP_ARRAY_TO_LIST(ARRAY_VERY_LARGE), 60) == 60 END

// to_seq

BEGIN BOOST_PP_SEQ_ELEM(0, BOOST_PP_ARRAY_TO_SEQ(ARRAY)) == 0 END
BEGIN BOOST_PP_SEQ_ELEM(3, BOOST_PP_ARRAY_TO_SEQ((5, (0, 1, 2, 3, 4)))) == 3 END
BEGIN BOOST_PP_SEQ_ELEM(17, BOOST_PP_ARRAY_TO_SEQ(ARRAY_LARGE)) == 17 END
BEGIN BOOST_PP_SEQ_ELEM(42, BOOST_PP_ARRAY_TO_SEQ((64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63)))) == 42 END

// to_tuple

# if BOOST_PP_VARIADICS

BEGIN BOOST_PP_TUPLE_ELEM(2, BOOST_PP_ARRAY_TO_TUPLE(ARRAY)) == 2 END
BEGIN BOOST_PP_TUPLE_ELEM(1, BOOST_PP_ARRAY_TO_TUPLE((5, (0, 1, 2, 3, 4)))) == 1 END
BEGIN BOOST_PP_TUPLE_ELEM(26, BOOST_PP_ARRAY_TO_TUPLE(ARRAY_LARGE)) == 26 END
BEGIN BOOST_PP_TUPLE_ELEM(37, BOOST_PP_ARRAY_TO_TUPLE((64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63)))) == 37 END

# else

BEGIN BOOST_PP_TUPLE_ELEM(3, 2, BOOST_PP_ARRAY_TO_TUPLE(ARRAY)) == 2 END
BEGIN BOOST_PP_TUPLE_ELEM(5, 1, BOOST_PP_ARRAY_TO_TUPLE((5, (0, 1, 2, 3, 4)))) == 1 END
BEGIN BOOST_PP_TUPLE_ELEM(33, 26, BOOST_PP_ARRAY_TO_TUPLE(ARRAY_LARGE)) == 26 END
BEGIN BOOST_PP_TUPLE_ELEM(64, 37, BOOST_PP_ARRAY_TO_TUPLE((64, (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63)))) == 37 END

# endif
