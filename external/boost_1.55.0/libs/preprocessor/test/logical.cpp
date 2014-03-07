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
# include <boost/preprocessor/logical.hpp>
# include <libs/preprocessor/test/test.h>

BEGIN BOOST_PP_NOT(0) == 1 END
BEGIN BOOST_PP_NOT(2) == 0 END

BEGIN BOOST_PP_AND(0, 0) == 0 END
BEGIN BOOST_PP_AND(0, 3) == 0 END
BEGIN BOOST_PP_AND(4, 0) == 0 END
BEGIN BOOST_PP_AND(5, 6) == 1 END

BEGIN BOOST_PP_OR(0, 0) == 0 END
BEGIN BOOST_PP_OR(0, 7) == 1 END
BEGIN BOOST_PP_OR(8, 0) == 1 END
BEGIN BOOST_PP_OR(9, 1) == 1 END

BEGIN BOOST_PP_XOR(0, 0) == 0 END
BEGIN BOOST_PP_XOR(0, 2) == 1 END
BEGIN BOOST_PP_XOR(3, 0) == 1 END
BEGIN BOOST_PP_XOR(4, 5) == 0 END

BEGIN BOOST_PP_NOR(0, 0) == 1 END
BEGIN BOOST_PP_NOR(0, 6) == 0 END
BEGIN BOOST_PP_NOR(7, 0) == 0 END
BEGIN BOOST_PP_NOR(8, 9) == 0 END
