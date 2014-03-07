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
# include <boost/preprocessor/debug.hpp>
# include <libs/preprocessor/test/test.h>

BEGIN sizeof(BOOST_PP_ASSERT_MSG(0, "text") "") / sizeof(char) != 1 END
BEGIN sizeof(BOOST_PP_ASSERT_MSG(1, "text") "") / sizeof(char) == 1 END

BOOST_PP_ASSERT(10)

# line BOOST_PP_LINE(100, __FILE__)
BEGIN __LINE__ == 100 END
