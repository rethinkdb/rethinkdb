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
# include <boost/preprocessor/selection.hpp>
# include <libs/preprocessor/test/test.h>

BEGIN BOOST_PP_MAX(2, 2) == 2 END
BEGIN BOOST_PP_MAX(2, 1) == 2 END
BEGIN BOOST_PP_MAX(1, 2) == 2 END

BEGIN BOOST_PP_MIN(2, 2) == 2 END
BEGIN BOOST_PP_MIN(2, 1) == 1 END
BEGIN BOOST_PP_MIN(1, 2) == 1 END
