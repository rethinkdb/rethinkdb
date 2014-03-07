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
# /* This example uses the preprocessor library to implement a generalized
#  * macro for implementing Duff's Device.
#  *
#  * This example was inspired by an original generalized macro for
#  * for implementing Duff's Device written by Joerg Walter.
#  */
#
# include <assert.h>
#
# include <boost/preprocessor/repetition/repeat.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* Expands to a Duff's Device. */
# define DUFFS_DEVICE(UNROLLING_FACTOR, COUNTER_TYPE, N, STATEMENT) \
   do { \
      COUNTER_TYPE duffs_device_initial_cnt = (N); \
      if (duffs_device_initial_cnt > 0) { \
         COUNTER_TYPE duffs_device_running_cnt = (duffs_device_initial_cnt + (UNROLLING_FACTOR - 1)) / UNROLLING_FACTOR; \
         switch (duffs_device_initial_cnt % UNROLLING_FACTOR) { \
            do { \
               BOOST_PP_REPEAT(UNROLLING_FACTOR, DUFFS_DEVICE_C, (UNROLLING_FACTOR, { STATEMENT })) \
            } while (--duffs_device_running_cnt); \
         } \
      } \
   } while (0) \
   /**/
#
# define DUFFS_DEVICE_C(Z, I, UNROLLING_FACTOR_STATEMENT) \
   case (I ? BOOST_PP_TUPLE_ELEM(2, 0, UNROLLING_FACTOR_STATEMENT) - I : 0): \
      BOOST_PP_TUPLE_ELEM(2, 1, UNROLLING_FACTOR_STATEMENT); \
   /**/
#
# ifndef UNROLLING_FACTOR
# define UNROLLING_FACTOR 16
# endif
#
# ifndef N
# define N 1000
# endif

int main(void) {
   int i = 0;
   DUFFS_DEVICE(UNROLLING_FACTOR, int, 0, ++i;);
   assert(i == 0);
   DUFFS_DEVICE(UNROLLING_FACTOR, int, N, ++i;);
   assert(i == N);
   return 0;
}
