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
# if !BOOST_PP_IS_ITERATING
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/iteration.hpp>
# include <libs/preprocessor/test/test.h>
#
# define NO_FLAGS
#
# define BOOST_PP_ITERATION_PARAMS_1 (3, (1, 10, <libs/preprocessor/test/iteration.h>))
# include BOOST_PP_ITERATE()
#
# undef NO_FLAGS
#
# define BOOST_PP_ITERATION_PARAMS_1 (4, (1, 5, <libs/preprocessor/test/iteration.h>, 0x0001))
# include BOOST_PP_ITERATE()
#
# define BOOST_PP_ITERATION_PARAMS_1 (4, (1, 5, <libs/preprocessor/test/iteration.h>, 0x0002))
# include BOOST_PP_ITERATE()
#
# else
# if defined NO_FLAGS

struct BOOST_PP_CAT(X, BOOST_PP_ITERATION()) {
    BEGIN
        BOOST_PP_ITERATION() >= BOOST_PP_ITERATION_START() &&
        BOOST_PP_ITERATION() <= BOOST_PP_ITERATION_FINISH()
    END
};

# elif BOOST_PP_ITERATION_DEPTH() == 1 \
    && BOOST_PP_ITERATION_FLAGS() == 0x0001

struct BOOST_PP_CAT(Y, BOOST_PP_ITERATION()) { };

# elif BOOST_PP_ITERATION_DEPTH() == 1 \
    && BOOST_PP_ITERATION_FLAGS() == 0x0002

# define BOOST_PP_ITERATION_PARAMS_2 (3, (1, BOOST_PP_ITERATION(), <libs/preprocessor/test/iteration.h>))
# include BOOST_PP_ITERATE()

# elif BOOST_PP_ITERATION_DEPTH() == 2 \
    && BOOST_PP_FRAME_FLAGS(1) == 0x0002

struct BOOST_PP_CAT(Z, BOOST_PP_CAT(BOOST_PP_ITERATION(), BOOST_PP_RELATIVE_ITERATION(1))) { };

# else
#
# error should not get here!
#
# endif
# endif
