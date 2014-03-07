/* test_lagged_fibonacci.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_lagged_fibonacci.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/lagged_fibonacci.hpp>

typedef boost::random::lagged_fibonacci_engine<boost::uint32_t, 24, 607, 273> lagged_fibonacci;
#define BOOST_RANDOM_URNG lagged_fibonacci

#define BOOST_RANDOM_SEED_WORDS 607

#define BOOST_RANDOM_VALIDATION_VALUE 3543833U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 1364481U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 4372778U

#define BOOST_RANDOM_GENERATE_VALUES { 0xF61A5094U, 0xFC4BA046U, 0xF1C41E92U, 0x3D82FE61U }

#include "test_generator.ipp"
