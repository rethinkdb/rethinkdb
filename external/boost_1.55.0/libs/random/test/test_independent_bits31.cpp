/* test_independent_bits31.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_independent_bits31.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/independent_bits.hpp>
#include <boost/random/linear_congruential.hpp>

typedef boost::random::independent_bits_engine<boost::random::minstd_rand0, 31, boost::uint32_t> independent_bits31;
#define BOOST_RANDOM_URNG independent_bits31

#define BOOST_RANDOM_SEED_WORDS 1

#define BOOST_RANDOM_VALIDATION_VALUE 26292962U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 364481529U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 1399154219U

#define BOOST_RANDOM_GENERATE_VALUES { 0xC1A63AF0U, 0xD66C0614U, 0xADE076B1U, 0xC1DAE13FU }

#include "test_generator.ipp"
