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
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/comparison/equal.hpp>
# include <boost/preprocessor/comparison/not_equal.hpp>
# include <boost/preprocessor/facilities/intercept.hpp>
# include <boost/preprocessor/repetition.hpp>
# include <libs/preprocessor/test/test.h>

# define MAX 10

# define NTH(z, n, data) data ## n

int add(BOOST_PP_ENUM_PARAMS(MAX, int x)) {
    return BOOST_PP_REPEAT(MAX, NTH, + x);
}

const int r = add(BOOST_PP_ENUM_PARAMS(MAX, 1 BOOST_PP_INTERCEPT));

# define CONSTANT(z, n, text) BOOST_PP_CAT(text, n) = n
const int BOOST_PP_ENUM(MAX, CONSTANT, default_param_);

# define TEST(n) \
    void BOOST_PP_CAT(test_enum_params, n)(BOOST_PP_ENUM_PARAMS(n, int x)); \
    void BOOST_PP_CAT(test_enum_params_with_a_default, n)(BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(n, int x, 0)); \
    void BOOST_PP_CAT(test_enum_params_with_defaults, n)(BOOST_PP_ENUM_PARAMS_WITH_DEFAULTS(n, int x, default_param_));

TEST(0)
TEST(MAX)

template<BOOST_PP_ENUM_PARAMS(MAX, class T)> struct no_rescan;

# define F1(z, n, p) p n
BEGIN 1 + (4+5+6) BOOST_PP_REPEAT_FROM_TO(4, 7, F1, -) END

# define PRED(r, state) BOOST_PP_NOT_EQUAL(state, BOOST_PP_INC(MAX))
# define OP(r, state) BOOST_PP_INC(state)
# define MACRO(r, state) BOOST_PP_COMMA_IF(BOOST_PP_NOT_EQUAL(state, 1)) BOOST_PP_CAT(class T, state)

template<BOOST_PP_FOR(1, PRED, OP, MACRO)> struct for_test;
