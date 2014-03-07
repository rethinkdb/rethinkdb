/* test_minstd_rand.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_minstd_rand.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/linear_congruential.hpp>
#include <boost/cstdint.hpp>

#define BOOST_RANDOM_URNG boost::random::minstd_rand

#define BOOST_RANDOM_SEED_WORDS 1

// validation values from the publications
#define BOOST_RANDOM_VALIDATION_VALUE 399268537U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 1000962296U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 182651141U

#define BOOST_RANDOM_GENERATE_VALUES { 0x8400BC8EU, 0xF45B895FU, 0x145F0F91U, 0xE5F8F088U }

#include "test_generator.ipp"
