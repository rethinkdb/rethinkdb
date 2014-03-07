/* test_mt19937.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_mt19937.cpp 74867 2011-10-09 23:13:31Z steven_watanabe $
 *
 */

#include <boost/random/mersenne_twister.hpp>
#include <algorithm>
#include <vector>
#include <boost/cstdint.hpp>

#define BOOST_RANDOM_URNG boost::random::mt19937

#define BOOST_RANDOM_SEED_WORDS 624

// validation by experiment from mt19937.c
#define BOOST_RANDOM_VALIDATION_VALUE 4123659995U
#define BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE 666528879U
#define BOOST_RANDOM_ITERATOR_VALIDATION_VALUE 3408548740U

#define BOOST_RANDOM_GENERATE_VALUES { 0xD091BB5CU, 0x22AE9EF6U, 0xE7E1FAEEU, 0xD5C31F79U }

#include "test_generator.ipp"

struct seed_seq_0 {
    template<class It>
    void generate(It begin, It end) const {
        std::fill(begin, end, boost::uint32_t(0));
    }
};

struct seed_seq_1 {
    template<class It>
    void generate(It begin, It end) const {
        std::fill(begin, end, boost::uint32_t(0));
        *(end - 1) = 1;
    }
};

BOOST_AUTO_TEST_CASE(test_special_seed) {
    {
    seed_seq_1 seed;
    std::vector<boost::uint32_t> vec(624);
    seed.generate(vec.begin(), vec.end());
    
    std::vector<boost::uint32_t>::iterator it = vec.begin();
    boost::mt19937 gen1(it, vec.end());
    BOOST_CHECK_EQUAL(gen1(), 0);
    BOOST_CHECK_EQUAL(gen1(), 0);
    
    boost::mt19937 gen2(seed);
    BOOST_CHECK_EQUAL(gen2(), 0);
    BOOST_CHECK_EQUAL(gen2(), 0);

    BOOST_CHECK_EQUAL(gen1, gen2);
    }
    {
    seed_seq_0 seed;
    std::vector<boost::uint32_t> vec(624);
    seed.generate(vec.begin(), vec.end());
    
    std::vector<boost::uint32_t>::iterator it = vec.begin();
    boost::mt19937 gen1(it, vec.end());
    BOOST_CHECK_EQUAL(gen1(), 1141379330u);
    BOOST_CHECK_EQUAL(gen1(), 0);
    
    boost::mt19937 gen2(seed);
    BOOST_CHECK_EQUAL(gen2(), 1141379330u);
    BOOST_CHECK_EQUAL(gen2(), 0);

    BOOST_CHECK_EQUAL(gen1, gen2);
    }
}
