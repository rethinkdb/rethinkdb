# /* Copyright (C) 2002
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
#
# /* See http://www.boost.org for most recent version. */
#
# /* The time complexity of DELAY(N) is O(2^N).
#  *
#  * Handy when recompiles are too fast to take a coffee break. :)
#  *
#  * Template metaprogramming can be used for implementing similar
#  * delays. Unfortunately template instantiation consumes memory,
#  * therefore compilers usually fail to fully compile long template
#  * based delays, because they run out of memory.
#  *
#  * On many compilers (e.g. g++, MSVC++), this macro takes only a
#  * small amount of memory to preprocess. On some compilers (e.g.
#  * MWCW), however, this macro seems to consume huge amounts of
#  * memory.
#  */
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/facilities/empty.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# ifndef DELAY_MAX
# define DELAY_MAX 14
# endif
#
# define DELAY(N) BOOST_PP_TUPLE_ELEM(2, 0, (BOOST_PP_EMPTY, BOOST_PP_WHILE(DELAY_C, BOOST_PP_CAT(DELAY_F, N), BOOST_PP_DEC(N))))()
#
# define DELAY_C(D, N) N
#
# define DELAY_F0(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F1(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F2(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F3(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F4(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F5(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F6(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F7(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F8(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F9(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F10(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F11(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F12(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F13(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F14(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F15(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F16(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F17(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F18(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F19(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F20(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F21(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F22(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F23(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F24(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F25(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F26(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F27(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F28(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F29(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F30(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F31(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F32(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F33(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F34(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F35(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F36(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F37(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F38(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F39(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F40(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F41(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F42(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F43(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F44(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F45(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F46(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F47(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F48(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F49(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))
# define DELAY_F50(D, N) BOOST_PP_IF(1, BOOST_PP_DEC(N), BOOST_PP_WHILE_ ## D(DELAY_C, DELAY_F ## N, BOOST_PP_DEC(N)))

DELAY(DELAY_MAX)
