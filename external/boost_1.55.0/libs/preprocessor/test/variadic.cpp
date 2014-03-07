# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2011.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# include <boost/preprocessor/variadic.hpp>
# include <boost/preprocessor/array/size.hpp>
# include <boost/preprocessor/array/elem.hpp>
# include <boost/preprocessor/list/at.hpp>
# include <boost/preprocessor/list/size.hpp>
# include <boost/preprocessor/seq/elem.hpp>
# include <boost/preprocessor/seq/size.hpp>
# include <boost/preprocessor/tuple/size.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <libs/preprocessor/test/test.h>

#if BOOST_PP_VARIADICS

#define VDATA 0,1,2,3,4,5,6
#define VDATA_LARGE 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
#define VDATA_VERY_LARGE 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63

BEGIN BOOST_PP_VARIADIC_ELEM(4,VDATA) == 4 END
BEGIN BOOST_PP_VARIADIC_ELEM(6,7,11,3,8,14,85,56,92,165) == 56 END
BEGIN BOOST_PP_VARIADIC_ELEM(29,VDATA_LARGE) == 29 END
BEGIN BOOST_PP_VARIADIC_ELEM(57,VDATA_VERY_LARGE) == 57 END
BEGIN BOOST_PP_VARIADIC_ELEM(35, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63) == 35 END
BEGIN BOOST_PP_VARIADIC_SIZE(VDATA) == 7 END
BEGIN BOOST_PP_VARIADIC_SIZE(7,11,3,8,14,85,56,92,165) == 9 END
BEGIN BOOST_PP_VARIADIC_SIZE(VDATA_LARGE) == 33 END
BEGIN BOOST_PP_VARIADIC_SIZE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32) == 33 END
BEGIN BOOST_PP_VARIADIC_SIZE(VDATA_VERY_LARGE) == 64 END
BEGIN BOOST_PP_ARRAY_SIZE(BOOST_PP_VARIADIC_TO_ARRAY(VDATA)) == 7 END
BEGIN BOOST_PP_ARRAY_SIZE(BOOST_PP_VARIADIC_TO_ARRAY(VDATA_VERY_LARGE)) == 64 END
BEGIN BOOST_PP_ARRAY_ELEM(4,BOOST_PP_VARIADIC_TO_ARRAY(7,11,3,8,14,85,56,92,165)) == 14 END
BEGIN BOOST_PP_ARRAY_ELEM(30,BOOST_PP_VARIADIC_TO_ARRAY(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)) == 30 END
BEGIN BOOST_PP_LIST_AT(BOOST_PP_VARIADIC_TO_LIST(VDATA),3) == 3 END
BEGIN BOOST_PP_LIST_AT(BOOST_PP_VARIADIC_TO_LIST(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63),49) == 49 END
BEGIN BOOST_PP_LIST_SIZE(BOOST_PP_VARIADIC_TO_LIST(7,11,3,8,14,85,56,92,165)) == 9 END
BEGIN BOOST_PP_LIST_SIZE(BOOST_PP_VARIADIC_TO_LIST(VDATA_LARGE)) == 33 END
BEGIN BOOST_PP_SEQ_ELEM(5,BOOST_PP_VARIADIC_TO_SEQ(VDATA)) == 5 END
BEGIN BOOST_PP_SEQ_ELEM(16,BOOST_PP_VARIADIC_TO_SEQ(VDATA_LARGE)) == 16 END
BEGIN BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_TO_SEQ(3,78,22,11,3)) == 5 END
BEGIN BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_TO_SEQ(VDATA_VERY_LARGE)) == 64 END
BEGIN BOOST_PP_TUPLE_SIZE(BOOST_PP_VARIADIC_TO_TUPLE(VDATA)) == 7 END
BEGIN BOOST_PP_TUPLE_SIZE(BOOST_PP_VARIADIC_TO_TUPLE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63)) == 64 END
BEGIN BOOST_PP_TUPLE_ELEM(8,BOOST_PP_VARIADIC_TO_TUPLE(7,11,3,8,14,85,56,92,165)) == 165 END
BEGIN BOOST_PP_TUPLE_ELEM(27,BOOST_PP_VARIADIC_TO_TUPLE(VDATA_LARGE)) == 27 END

#endif
