/* test_mt119937_64.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_mt19937_64.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/mersenne_twister.hpp>
#include <boost/cstdint.hpp>

#define BOOST_RANDOM_URNG boost::random::mt19937_64

#define BOOST_RANDOM_SEED_WORDS 624

// validation from the C++0x draft (n3090)
#define BOOST_RANDOM_VALIDATION_VALUE UINT64_C(9981545732273789042)
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE UINT64_C(12176471137395770412)
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE UINT64_C(13543700832025962283)

#define BOOST_RANDOM_GENERATE_VALUES { 0xF6F6AEA6U, 0xC96D191CU, 0x8BC80F1CU, 0x401F7AC7U }

#include "test_generator.ipp"
