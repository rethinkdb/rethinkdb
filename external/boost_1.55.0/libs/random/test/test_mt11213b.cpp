/* test_mt11213b.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_mt11213b.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/mersenne_twister.hpp>

#define BOOST_RANDOM_URNG boost::random::mt11213b

#define BOOST_RANDOM_SEED_WORDS 351

#define BOOST_RANDOM_VALIDATION_VALUE 3809585648U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 2936939529U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 2434563197U

#define BOOST_RANDOM_GENERATE_VALUES { 0xEF3F3F3FU, 0x70082175U, 0xDAF6EAF5U, 0x2A16A63EU }

#include "test_generator.ipp"
