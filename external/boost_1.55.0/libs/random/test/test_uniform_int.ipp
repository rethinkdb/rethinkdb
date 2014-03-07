/* boost test_uniform_int.ipp
 *
 * Copyright Jens Maurer 2000
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_uniform_int.ipp 71018 2011-04-05 21:27:52Z steven_watanabe $
 */

#include <numeric>
#include <sstream>
#include <vector>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/lagged_fibonacci.hpp>
#include <boost/random/variate_generator.hpp>
#include "chi_squared_test.hpp"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

template<class Generator>
void check_uniform_int(Generator & gen, int iter)
{
    int range = (gen.max)()-(gen.min)()+1;
    std::vector<int> bucket(range);
    for(int j = 0; j < iter; j++) {
        int result = gen();
        BOOST_CHECK_GE(result, (gen.min)());
        BOOST_CHECK_LE(result, (gen.max)());
        if(result >= (gen.min)() && result <= (gen.max)()) {
            bucket[result-(gen.min)()]++;
        }
    }
    int sum = std::accumulate(bucket.begin(), bucket.end(), 0);
    std::vector<double> expected(range, 1.0 / range);
    BOOST_CHECK_LT(chi_squared_test(bucket, expected, sum), 0.99);
}

BOOST_AUTO_TEST_CASE(test_uniform_int)
{
    boost::random::mt19937 gen;
    typedef BOOST_RANDOM_UNIFORM_INT<int> int_gen;

    // large range => small range (modulo case)
    typedef boost::random::variate_generator<boost::random::mt19937&, int_gen> level_one;

    level_one uint12(gen, int_gen(1,2));
    BOOST_CHECK((uint12.distribution().min)() == 1);
    BOOST_CHECK((uint12.distribution().max)() == 2);
    check_uniform_int(uint12, 100000);
    level_one uint16(gen, int_gen(1,6));
    check_uniform_int(uint16, 100000);

    // test chaining to get all cases in operator()

    // identity map
    typedef boost::random::variate_generator<level_one&, int_gen> level_two;
    level_two uint01(uint12, int_gen(0, 1));
    check_uniform_int(uint01, 100000);

    // small range => larger range
    level_two uint05(uint12, int_gen(-3, 2));
    check_uniform_int(uint05, 100000);
  
    // small range => larger range
    level_two uint099(uint12, int_gen(0, 99));
    check_uniform_int(uint099, 100000);

    // larger => small range, rejection case
    typedef boost::random::variate_generator<level_two&, int_gen> level_three;
    level_three uint1_4(uint05, int_gen(1, 4));
    check_uniform_int(uint1_4, 100000);

    typedef BOOST_RANDOM_UNIFORM_INT<boost::uint8_t> int8_gen;
    typedef boost::random::variate_generator<boost::random::mt19937&, int8_gen> gen8_t;

    gen8_t gen8_03(gen, int8_gen(0, 3));

    // use the full range of the type, where the destination
    // range is a power of the source range
    typedef boost::random::variate_generator<gen8_t, int8_gen> uniform_uint8;
    uniform_uint8 uint8_0255(gen8_03, int8_gen(0, 255));
    check_uniform_int(uint8_0255, 100000);

    // use the full range, but a generator whose range is not
    // a root of the destination range.
    gen8_t gen8_02(gen, int8_gen(0, 2));
    uniform_uint8 uint8_0255_2(gen8_02, int8_gen(0, 255));
    check_uniform_int(uint8_0255_2, 100000);

    // expand the range to a larger type.
    typedef boost::random::variate_generator<gen8_t, int_gen> uniform_uint_from8;
    uniform_uint_from8 uint0300(gen8_03, int_gen(0, 300));
    check_uniform_int(uint0300, 100000);
}

#if !defined(BOOST_NO_INT64_T) && !defined(BOOST_NO_INTEGRAL_INT64_T)

// testcase by Mario Rutti
class ruetti_gen
{
public:
    ruetti_gen() : state((max)() - 1) {}
    typedef boost::uint64_t result_type;
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 0; }
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return std::numeric_limits<result_type>::max BOOST_PREVENT_MACRO_SUBSTITUTION (); }
    result_type operator()() { return state--; }
private:
    result_type state;
};

BOOST_AUTO_TEST_CASE(test_overflow_range)
{
    ruetti_gen gen;
    BOOST_RANDOM_DISTRIBUTION dist(0, 10);
    for (int i=0;i<10;i++) {
        dist(gen);
    }
}

#endif

BOOST_AUTO_TEST_CASE(test_misc)
{
    // bug report from Ken Mahler:  This used to lead to an endless loop.
    typedef BOOST_RANDOM_UNIFORM_INT<unsigned int> uint_dist;
    boost::minstd_rand mr;
    boost::variate_generator<boost::minstd_rand, uint_dist> r2(mr,
                                                            uint_dist(0, 0xffffffff));
    r2();
    r2();

    // bug report from Fernando Cacciola:  This used to lead to an endless loop.
    // also from Douglas Gregor
    boost::variate_generator<boost::minstd_rand, BOOST_RANDOM_DISTRIBUTION > x(mr, BOOST_RANDOM_DISTRIBUTION(0, 8361));
    x();

    // bug report from Alan Stokes and others: this throws an assertion
    boost::variate_generator<boost::minstd_rand, BOOST_RANDOM_DISTRIBUTION > y(mr, BOOST_RANDOM_DISTRIBUTION(1,1));
    y();
    y();
    y();
}
