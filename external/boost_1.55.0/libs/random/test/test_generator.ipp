/* test_generator.ipp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_generator.ipp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include "concepts.hpp"
#include <boost/random/seed_seq.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using boost::random::test::RandomNumberEngine;
BOOST_CONCEPT_ASSERT((RandomNumberEngine< BOOST_RANDOM_URNG >));

typedef BOOST_RANDOM_URNG::result_type result_type;
typedef boost::random::detail::seed_type<result_type>::type seed_type;

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif

template<class Converted, class URNG, class T>
void test_seed_conversion(URNG & urng, const T & t)
{
    Converted c = static_cast<Converted>(t);
    if(static_cast<T>(c) == t) {
        URNG urng2(c);
        std::ostringstream msg;
        msg << "Testing seed: type " << typeid(Converted).name() << ", value " << c;
        BOOST_CHECK_MESSAGE(urng == urng2, msg.str());
        urng2.seed(c);
        BOOST_CHECK_MESSAGE(urng == urng2, msg.str());
    }
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

void test_seed(seed_type value)
{
    BOOST_RANDOM_URNG urng(value);

    // integral types
    test_seed_conversion<char>(urng, value);
    test_seed_conversion<signed char>(urng, value);
    test_seed_conversion<unsigned char>(urng, value);
    test_seed_conversion<short>(urng, value);
    test_seed_conversion<unsigned short>(urng, value);
    test_seed_conversion<int>(urng, value);
    test_seed_conversion<unsigned int>(urng, value);
    test_seed_conversion<long>(urng, value);
    test_seed_conversion<unsigned long>(urng, value);
#if !defined(BOOST_NO_INT64_T)
    test_seed_conversion<boost::int64_t>(urng, value);
    test_seed_conversion<boost::uint64_t>(urng, value);
#endif

    // floating point types
    test_seed_conversion<float>(urng, value);
    test_seed_conversion<double>(urng, value);
    test_seed_conversion<long double>(urng, value);
}

BOOST_AUTO_TEST_CASE(test_default_seed)
{
    BOOST_RANDOM_URNG urng;
    BOOST_RANDOM_URNG urng2;
    urng2();
    BOOST_CHECK_NE(urng, urng2);
    urng2.seed();
    BOOST_CHECK_EQUAL(urng, urng2);
}

BOOST_AUTO_TEST_CASE(test_arithmetic_seed)
{
    test_seed(static_cast<seed_type>(0));
    test_seed(static_cast<seed_type>(127));
    test_seed(static_cast<seed_type>(539157235));
    test_seed(static_cast<seed_type>(~0u));
}
   
BOOST_AUTO_TEST_CASE(test_iterator_seed)
{
    const std::vector<int> v((std::max)(std::size_t(9999u), sizeof(BOOST_RANDOM_URNG) / 4), 0x41);
    std::vector<int>::const_iterator it = v.begin();
    std::vector<int>::const_iterator it_end = v.end();
    BOOST_RANDOM_URNG urng(it, it_end);
    BOOST_CHECK(it != v.begin());
    std::iterator_traits<std::vector<int>::const_iterator>::difference_type n_words = (it - v.begin());
    BOOST_CHECK_GT(n_words, 0);
    BOOST_CHECK_EQUAL(n_words, BOOST_RANDOM_SEED_WORDS);

    it = v.begin();
    BOOST_RANDOM_URNG urng2;
    urng2.seed(it, it_end);
    std::iterator_traits<std::vector<int>::const_iterator>::difference_type n_words2 = (it - v.begin());
    BOOST_CHECK_EQUAL(n_words, n_words2);
    BOOST_CHECK_EQUAL(urng, urng2);

    it = v.end();
    BOOST_CHECK_THROW(BOOST_RANDOM_URNG(it, it_end), std::invalid_argument);
    BOOST_CHECK_THROW(urng.seed(it, it_end), std::invalid_argument);

    if(n_words > 1) {
        it = v.end();
        --it;
        BOOST_CHECK_THROW(BOOST_RANDOM_URNG(it, it_end), std::invalid_argument);
        it = v.end();
        --it;
        BOOST_CHECK_THROW(urng.seed(it, it_end), std::invalid_argument);
    }
}

BOOST_AUTO_TEST_CASE(test_seed_seq_seed)
{
    boost::random::seed_seq q;
    BOOST_RANDOM_URNG urng(q);
    BOOST_RANDOM_URNG urng2;
    BOOST_CHECK_NE(urng, urng2);
    urng2.seed(q);
    BOOST_CHECK_EQUAL(urng, urng2);
}

template<class CharT>
void do_test_streaming(const BOOST_RANDOM_URNG& urng)
{
    BOOST_RANDOM_URNG urng2;
    std::basic_ostringstream<CharT> output;
    output << urng;
    BOOST_CHECK_NE(urng, urng2);
    // restore old state
    std::basic_istringstream<CharT> input(output.str());
    input >> urng2;
    BOOST_CHECK_EQUAL(urng, urng2);
}

BOOST_AUTO_TEST_CASE(test_streaming)
{
    BOOST_RANDOM_URNG urng;
    urng.discard(9307);
    do_test_streaming<char>(urng);
#if !defined(BOOST_NO_STD_WSTREAMBUF) && !defined(BOOST_NO_STD_WSTRING)
    do_test_streaming<wchar_t>(urng);
#endif
}

BOOST_AUTO_TEST_CASE(test_discard)
{
    BOOST_RANDOM_URNG urng;
    BOOST_RANDOM_URNG urng2;
    BOOST_CHECK_EQUAL(urng, urng2);
    for(int i = 0; i < 9307; ++i)
        urng();
    BOOST_CHECK_NE(urng, urng2);
    urng2.discard(9307);
    BOOST_CHECK_EQUAL(urng, urng2);
}

BOOST_AUTO_TEST_CASE(test_copy)
{
    BOOST_RANDOM_URNG urng;
    urng.discard(9307);
    {
        BOOST_RANDOM_URNG urng2 = urng;
        BOOST_CHECK_EQUAL(urng, urng2);
    }
    {
        BOOST_RANDOM_URNG urng2(urng);
        BOOST_CHECK_EQUAL(urng, urng2);
    }
    {
        BOOST_RANDOM_URNG urng2;
        urng2 = urng;
        BOOST_CHECK_EQUAL(urng, urng2);
    }
}

BOOST_AUTO_TEST_CASE(test_min_max)
{
    BOOST_RANDOM_URNG urng;
    for(int i = 0; i < 10000; ++i) {
        result_type value = urng();
        BOOST_CHECK_GE(value, (BOOST_RANDOM_URNG::min)());
        BOOST_CHECK_LE(value, (BOOST_RANDOM_URNG::max)());
    }
}

BOOST_AUTO_TEST_CASE(test_comparison)
{
    BOOST_RANDOM_URNG urng;
    BOOST_RANDOM_URNG urng2;
    BOOST_CHECK(urng == urng2);
    BOOST_CHECK(!(urng != urng2));
    urng();
    BOOST_CHECK(urng != urng2);
    BOOST_CHECK(!(urng == urng2));
}

BOOST_AUTO_TEST_CASE(validate)
{
    BOOST_RANDOM_URNG urng;
    for(int i = 0; i < 9999; ++i) {
        urng();
    }
    BOOST_CHECK_EQUAL(urng(), BOOST_RANDOM_VALIDATION_VALUE);
}

BOOST_AUTO_TEST_CASE(validate_seed_seq)
{
    boost::random::seed_seq seed;
    BOOST_RANDOM_URNG urng(seed);
    for(int i = 0; i < 9999; ++i) {
        urng();
    }
    BOOST_CHECK_EQUAL(urng(), BOOST_RANDOM_SEED_SEQ_VALIDATION_VALUE);
}

BOOST_AUTO_TEST_CASE(validate_iter)
{
    const std::vector<int> v((std::max)(std::size_t(9999u), sizeof(BOOST_RANDOM_URNG) / 4), 0x41);
    std::vector<int>::const_iterator it = v.begin();
    std::vector<int>::const_iterator it_end = v.end();
    BOOST_RANDOM_URNG urng(it, it_end);
    for(int i = 0; i < 9999; ++i) {
        urng();
    }
    BOOST_CHECK_EQUAL(urng(), BOOST_RANDOM_ITERATOR_VALIDATION_VALUE);
}

BOOST_AUTO_TEST_CASE(test_generate)
{
    BOOST_RANDOM_URNG urng;
    boost::uint32_t expected[] = BOOST_RANDOM_GENERATE_VALUES;
    static const std::size_t N = sizeof(expected)/sizeof(expected[0]); 
    boost::uint32_t actual[N];
    urng.generate(&actual[0], &actual[0] + N);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual, actual + N, expected, expected + N);
}
