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
# include <boost/preprocessor/slot.hpp>
# include <libs/preprocessor/test/test.h>

# define X() 4

# define BOOST_PP_VALUE 1 + 2 + 3 + X()
# include BOOST_PP_ASSIGN_SLOT(1)

# undef X

BEGIN BOOST_PP_SLOT(1) == 10 END

# define BOOST_PP_VALUE BOOST_PP_SLOT(1) * BOOST_PP_SLOT(1)
# include BOOST_PP_ASSIGN_SLOT(1)

BEGIN BOOST_PP_SLOT(1) == 100 END
