/* test_ecuyer1988.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_ecuyer1988.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/additive_combine.hpp>

#define BOOST_RANDOM_URNG boost::random::ecuyer1988

#define BOOST_RANDOM_SEED_WORDS 2

#define BOOST_RANDOM_VALIDATION_VALUE 2060321752U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 1928563088U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 776923198U

#define BOOST_RANDOM_GENERATE_VALUES { 0x7AE0C087U, 0x948A8A31U, 0xBE5CCBA9U, 0x1316692CU }

#include "test_generator.ipp"
