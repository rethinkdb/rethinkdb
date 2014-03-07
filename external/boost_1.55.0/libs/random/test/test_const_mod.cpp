/* test_const_mod.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_const_mod.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/detail/const_mod.hpp>

#include <boost/cstdint.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

typedef boost::mpl::vector<
    boost::int8_t,
    boost::uint8_t
> int8_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_mult8, IntType, int8_types) {
    for(int i = 0; i < 127; ++i) {
        for(int j = 0; j < 127; ++j) {
            BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 127>::mult(IntType(i), IntType(j))), i * j % 127);
        }
    }
    int modulus = (std::numeric_limits<IntType>::max)() + 1;
    for(int i = 0; i < modulus; ++i) {
        for(int j = 0; j < modulus; ++j) {
            BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 0>::mult(IntType(i), IntType(j))), i * j % modulus);
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_add8, IntType, int8_types) {
    for(int i = 0; i < 127; ++i) {
        for(int j = 0; j < 127; ++j) {
            BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 127>::add(IntType(i), IntType(j))), (i + j) % 127);
        }
    }
    {
    const int modulus = boost::integer_traits<IntType>::const_max;
    for(int i = 0; i < modulus; ++i) {
        for(int j = 0; j < modulus; ++j) {
            BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, modulus>::add(IntType(i), IntType(j))), (i + j) % modulus);
        }
    }
    }
    {
    int modulus = (std::numeric_limits<IntType>::max)() + 1;
    for(int i = 0; i < modulus; ++i) {
        for(int j = 0; j < modulus; ++j) {
            BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 0>::add(IntType(i), IntType(j))), (i + j) % modulus);
        }
    }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_mult_add8, IntType, int8_types) {
    for(int i = 0; i < 127; i += 5) {
        for(int j = 0; j < 127; j += 3) {
            for(int k = 0; k < 127; k += 3) {
                BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 127>::mult_add(IntType(i), IntType(j), IntType(k))), (i * j + k) % 127);
            }
        }
    }
    {
    int modulus = (std::numeric_limits<IntType>::max)() + 1;
    for(int i = 0; i < modulus; i += 5) {
        for(int j = 0; j < modulus; j += 3) {
            for(int k = 0; k < modulus; k += 3) {
                BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 0>::mult_add(IntType(i), IntType(j), IntType(k))), (i * j + k) % modulus);
            }
        }
    }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_invert8, IntType, int8_types) {
    for(int i = 1; i < 127; ++i) {
        IntType inverse = boost::random::const_mod<IntType, 127>::invert(IntType(i));
        BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 127>::mult(IntType(i), inverse)), 1);
    }
    int modulus = (std::numeric_limits<IntType>::max)() + 1;
    for(int i = 1; i < modulus; i += 2) {
        IntType inverse = boost::random::const_mod<IntType, 0>::invert(IntType(i));
        BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 0>::mult(IntType(i), inverse)), 1);
    }
}

typedef boost::mpl::vector<
    boost::int32_t,
    boost::uint32_t
> int32_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_mult32, IntType, int32_types) {
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult(0, 0)), IntType(0));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult(0, 2147483562)), IntType(0));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult(2147483562, 0)), IntType(0));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult(2147483562, 2147483562)), IntType(1));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult(1234567890, 1234657890)), IntType(813106682));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_add32, IntType, int32_types) {
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::add(0, 0)), IntType(0));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::add(0, 2147483562)), IntType(2147483562));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::add(2147483562, 0)), IntType(2147483562));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::add(2147483562, 2147483562)), IntType(2147483561));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::add(1234567890, 1234657890)), IntType(321742217));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_mult_add32, IntType, int32_types) {
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult_add(0, 0, 0)), IntType(0));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult_add(0, 2147483562, 827364)), IntType(827364));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult_add(2147483562, 0, 827364)), IntType(827364));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult_add(2147483562, 2147483562, 2147483562)), IntType(0));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult_add(1234567890, 1234657890, 1726384759)), IntType(392007878));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_invert32, IntType, int32_types) {
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::invert(0)), IntType(0));
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult_add(0, 0, 0)), IntType(0));
    IntType inverse;
    inverse = boost::random::const_mod<IntType, 2147483563>::invert(2147483562);
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult(2147483562, inverse)), IntType(1));
    inverse = boost::random::const_mod<IntType, 2147483563>::invert(1234567890);
    BOOST_CHECK_EQUAL((boost::random::const_mod<IntType, 2147483563>::mult(1234567890, inverse)), IntType(1));
}

#if !defined(BOOST_NO_INT64_T)

typedef boost::mpl::vector<
    boost::int64_t,
    boost::uint64_t
> int64_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_mult64, IntType, int64_types) {
    typedef boost::random::const_mod<IntType, INT64_C(2147483563652738498)> const_mod_type;
    BOOST_CHECK_EQUAL((const_mod_type::mult(0, 0)), IntType(0));
    BOOST_CHECK_EQUAL((const_mod_type::mult(0, 2147483562)), IntType(0));
    BOOST_CHECK_EQUAL((const_mod_type::mult(2147483562, 0)), IntType(0));
    BOOST_CHECK_EQUAL((const_mod_type::mult(2147483562, 2147483562)), IntType(INT64_C(316718521754730848)));
    BOOST_CHECK_EQUAL((const_mod_type::mult(1234567890, 1234657890)), IntType(INT64_C(1524268986129152100)));
    BOOST_CHECK_EQUAL((const_mod_type::mult(INT64_C(1234567890726352938), INT64_C(1234657890736453927))), IntType(INT64_C(88656187017794672)));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_add64, IntType, int64_types) {
    typedef boost::random::const_mod<IntType, INT64_C(2147483563652738498)> const_mod_type;
    BOOST_CHECK_EQUAL((const_mod_type::add(0, 0)), IntType(0));
    BOOST_CHECK_EQUAL((const_mod_type::add(0, 2147483562)), IntType(2147483562));
    BOOST_CHECK_EQUAL((const_mod_type::add(2147483562, 0)), IntType(2147483562));
    BOOST_CHECK_EQUAL((const_mod_type::add(2147483562, 2147483562)), IntType(4294967124U));
    BOOST_CHECK_EQUAL((const_mod_type::add(1234567890, 1234657890)), IntType(2469225780U));
    BOOST_CHECK_EQUAL((const_mod_type::add(INT64_C(1234567890726352938), INT64_C(1234657890736453927))), IntType(INT64_C(321742217810068367)));
    BOOST_CHECK_EQUAL((const_mod_type::add(INT64_C(2147483563652738490), 8)), IntType(0));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_mult_add64, IntType, int64_types) {
    typedef boost::random::const_mod<IntType, INT64_C(2147483563652738498)> const_mod_type;
    BOOST_CHECK_EQUAL((const_mod_type::mult_add(0, 0, 0)), IntType(0));
    BOOST_CHECK_EQUAL((const_mod_type::mult_add(0, 2147483562, 827364)), IntType(827364));
    BOOST_CHECK_EQUAL((const_mod_type::mult_add(2147483562, 0, 827364)), IntType(827364));
    BOOST_CHECK_EQUAL((const_mod_type::mult_add(2147483562, 2147483562, 2147483562)), IntType(INT64_C(316718523902214410)));
    BOOST_CHECK_EQUAL((const_mod_type::mult_add(1234567890, 1234657890, 1726384759)), IntType(INT64_C(1524268987855536859)));
    BOOST_CHECK_EQUAL((const_mod_type::mult_add(INT64_C(1234567890726352938), INT64_C(1234657890736453927), INT64_C(1726384759726488649))), IntType(INT64_C(1815040946744283321)));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_invert64, IntType, int64_types) {
    typedef boost::random::const_mod<IntType, INT64_C(2147483563652738498)> const_mod_type;
    BOOST_CHECK_EQUAL((const_mod_type::invert(0)), IntType(0));
    BOOST_CHECK_EQUAL((const_mod_type::mult_add(0, 0, 0)), IntType(0));
    IntType inverse;
    inverse = const_mod_type::invert(INT64_C(7362947769));
    BOOST_CHECK_EQUAL((const_mod_type::mult(INT64_C(7362947769), inverse)), IntType(1));
    inverse = const_mod_type::invert(INT64_C(1263142436887493875));
    BOOST_CHECK_EQUAL((const_mod_type::mult(INT64_C(1263142436887493875), inverse)), IntType(1));
}

#endif
