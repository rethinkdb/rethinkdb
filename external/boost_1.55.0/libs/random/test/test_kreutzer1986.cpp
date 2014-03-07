/* test_kreutzer1986.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_kreutzer1986.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/shuffle_order.hpp>
#include <boost/cstdint.hpp>

#define BOOST_RANDOM_URNG boost::random::kreutzer1986

#define BOOST_RANDOM_SEED_WORDS 1

// validation by experiment from Harry Erwin's generator.h (private e-mail)
#define BOOST_RANDOM_VALIDATION_VALUE 139726U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 589731U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 163138U

#define BOOST_RANDOM_GENERATE_VALUES { 0x3EADAB08U, 0x85E481CEU, 0xCF84AEA5U, 0x39D4395BU }

#include "test_generator.ipp"
