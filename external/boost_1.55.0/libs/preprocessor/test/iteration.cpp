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
# if !BOOST_PP_IS_SELFISH
#
# include <libs/preprocessor/test/iteration.h>
#
# define TEST(n) BEGIN n == n END
#
# define BOOST_PP_LOCAL_MACRO(n) TEST(n)
# define BOOST_PP_LOCAL_LIMITS (1, 5)
# include BOOST_PP_LOCAL_ITERATE()
#
# define BOOST_PP_LOCAL_MACRO(n) TEST(n)
# define BOOST_PP_LOCAL_LIMITS (5, 1)
# include BOOST_PP_LOCAL_ITERATE()
#
# define BOOST_PP_INDIRECT_SELF <libs/preprocessor/test/iteration.cpp>
# include BOOST_PP_INCLUDE_SELF()
#
# else

BEGIN BOOST_PP_IS_SELFISH == 1 END

# endif
