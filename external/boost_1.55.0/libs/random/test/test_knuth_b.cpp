/* test_knuth_b.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_knuth_b.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/shuffle_order.hpp>
#include <boost/cstdint.hpp>

#define BOOST_RANDOM_URNG boost::random::knuth_b

#define BOOST_RANDOM_SEED_WORDS 1

// validation from the C++0x draft (n3090)
#define BOOST_RANDOM_VALIDATION_VALUE 1112339016U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 202352021U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 1692601883U

#define BOOST_RANDOM_GENERATE_VALUES { 0x5D189C63U, 0xD0544F0EU, 0x15B0E78FU, 0xD814D654U }

#include "test_generator.ipp"
