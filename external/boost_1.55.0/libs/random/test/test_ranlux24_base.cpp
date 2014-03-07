/* test_ranlux24_base.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_ranlux24_base.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/ranlux.hpp>

#define BOOST_RANDOM_URNG boost::random::ranlux24_base

#define BOOST_RANDOM_SEED_WORDS 24

// validation from the C++0x draft (n3126).
#define BOOST_RANDOM_VALIDATION_VALUE 7937952U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 14368281U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 7739608U

#define BOOST_RANDOM_GENERATE_VALUES { 0x55E57B2CU, 0xF2DEF915U, 0x6D1A0CD9U, 0xCA0109F9U }

#include "test_generator.ipp"
