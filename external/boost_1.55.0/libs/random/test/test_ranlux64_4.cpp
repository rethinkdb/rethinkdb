/* test_ranlux64_4.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_ranlux64_4.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/ranlux.hpp>
#include <boost/cstdint.hpp>
#include <cmath>

#define BOOST_RANDOM_URNG boost::random::ranlux64_4

#define BOOST_RANDOM_SEED_WORDS 48

// principal operation validated with CLHEP, values by experiment
#define BOOST_RANDOM_VALIDATION_VALUE UINT64_C(199461971133682)
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE UINT64_C(160535400540538)
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE UINT64_C(40074210927900)

#define BOOST_RANDOM_GENERATE_VALUES { 0xC35F616BU, 0xDC3C4DF1U, 0xF3F90D0AU, 0x206F9C9EU }

#include "test_generator.ipp"
