
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif

#include <boost/detail/lightweight_test.hpp>

#include <cmath>
#include <boost/functional/hash/detail/limits.hpp>
#include <boost/functional/hash/detail/float_functions.hpp>
#include <boost/detail/workaround.hpp>

#include <iostream>

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4723)   // conditional expression is constant
#if BOOST_MSVC < 1400
#pragma warning(disable:4267)   // conversion from 'size_t' to 'unsigned int',
                                // possible loss of data
#endif
#endif

#if defined(__GNUC__) && !defined(BOOST_INTEL_CXX_VERSION)
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

char const* float_type(float*) { return "float"; }
char const* float_type(double*) { return "double"; }
char const* float_type(long double*) { return "long double"; }

template <class T>
void float_tests(char const* name, T* = 0)
{
    std::cerr
        <<  "\n"
        <<  "Testing " BOOST_STRINGIZE(BOOST_HASH_TEST_NAMESPACE) "::hash<"
        <<  name
        <<  ">\n"
        <<  "\n"
        <<  "boost::hash_detail::limits<T>::digits = "
        <<  boost::hash_detail::limits<T>::digits<< "\n"
        <<  "boost::hash_detail::limits<int>::digits = "
        <<  boost::hash_detail::limits<int>::digits<< "\n"
        <<  "boost::hash_detail::limits<std::size_t>::digits = "
        <<  boost::hash_detail::limits<std::size_t>::digits
        <<  "\n"
        <<  "\n"
        <<  "boost::hash_detail::call_ldexp<T>::float_type = "
        <<  float_type(static_cast<BOOST_DEDUCED_TYPENAME
                boost::hash_detail::call_ldexp<T>::float_type*>(0))
        <<  "\n"
        <<  "boost::hash_detail::call_frexp<T>::float_type = "
        <<  float_type(static_cast<BOOST_DEDUCED_TYPENAME
                boost::hash_detail::call_frexp<T>::float_type*>(0))
        <<  "\n"
        <<  "boost::hash_detail::select_hash_type<T>::type = "
        <<  float_type(static_cast<BOOST_DEDUCED_TYPENAME
                boost::hash_detail::select_hash_type<T>::type*>(0))
        <<  "\n"
        <<  "\n"
        ;

    BOOST_HASH_TEST_NAMESPACE::hash<T> x1;

    T zero = 0;
    T minus_zero = (T) -1 * zero;

    BOOST_TEST(zero == minus_zero);
    BOOST_TEST(x1(zero) == x1(minus_zero));

#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(zero) == BOOST_HASH_TEST_NAMESPACE::hash_value(zero));
    BOOST_TEST(x1(minus_zero) == BOOST_HASH_TEST_NAMESPACE::hash_value(minus_zero));
#endif

    BOOST_TEST(x1(zero) != x1(0.5));
    BOOST_TEST(x1(minus_zero) != x1(0.5));
    BOOST_TEST(x1(0.5) != x1(-0.5));
    BOOST_TEST(x1(1) != x1(-1));

    using namespace std;

// Doing anything with infinity causes borland to crash.
#if defined(__BORLANDC__)
    std::cerr
        <<  "Not running infinity checks on Borland, as it causes it to crash."
            "\n";
#else
    if(boost::hash_detail::limits<T>::has_infinity) {
        T infinity = -log(zero);
        T infinity2 = (T) 1. / zero;
        T infinity3 = (T) -1. / minus_zero;
        T infinity4 = boost::hash_detail::limits<T>::infinity();

        T minus_infinity = log(zero);
        T minus_infinity2 = (T) -1. / zero;
        T minus_infinity3 = (T) 1. / minus_zero;

#if defined(BOOST_HASH_TEST_EXTENSIONS)
        BOOST_TEST(x1(infinity) == BOOST_HASH_TEST_NAMESPACE::hash_value(infinity));
        BOOST_TEST(x1(minus_infinity)
                == BOOST_HASH_TEST_NAMESPACE::hash_value(minus_infinity));
#endif

        if(infinity == infinity2)
            BOOST_TEST(x1(infinity) == x1(infinity2));
        if(infinity == infinity3)
            BOOST_TEST(x1(infinity) == x1(infinity3));
        if(infinity == infinity4)
            BOOST_TEST(x1(infinity) == x1(infinity4));

        if(minus_infinity == minus_infinity2)
            BOOST_TEST(x1(minus_infinity) == x1(minus_infinity2));
        if(minus_infinity == minus_infinity3)
            BOOST_TEST(x1(minus_infinity) == x1(minus_infinity3));

        BOOST_TEST(infinity != minus_infinity);

        if(x1(infinity) == x1(minus_infinity)) {
            std::cerr<<"x1(infinity) == x1(-infinity) == "<<x1(infinity)<<"\n";
        }

        // This should really be 'has_denorm == denorm_present' but some
        // compilers don't have 'denorm_present'. See also a later use.
        if(boost::hash_detail::limits<T>::has_denorm) {
            if(x1(boost::hash_detail::limits<T>::denorm_min()) == x1(infinity))
            {
                std::cerr
                    <<  "x1(denorm_min) == x1(infinity) == "
                    <<  x1(infinity)
                    <<  "\n";
            }

            if(x1(boost::hash_detail::limits<T>::denorm_min()) ==
                x1(minus_infinity))
            {
                std::cerr
                    <<  "x1(denorm_min) == x1(-infinity) == "
                    <<  x1(minus_infinity)
                    <<  "\n";
            }
        }

        if(boost::hash_detail::limits<T>::has_quiet_NaN) {
            if(x1(boost::hash_detail::limits<T>::quiet_NaN()) == x1(infinity))
            {
                std::cerr
                    <<  "x1(quiet_NaN) == x1(infinity) == "
                    <<  x1(infinity)
                    <<  "\n";
            }

            if(x1(boost::hash_detail::limits<T>::quiet_NaN()) ==
                x1(minus_infinity))
            {
                std::cerr
                    <<  "x1(quiet_NaN) == x1(-infinity) == "
                    <<  x1(minus_infinity)
                    <<  "\n";
            }
        }
    }
#endif

    T max = (boost::hash_detail::limits<T>::max)();
    T half_max = max / 2;
    T quarter_max = max / 4;
    T three_quarter_max = max - quarter_max;
    
    // Check the limits::max is in range.
    BOOST_TEST(max != half_max);
    BOOST_TEST(max != quarter_max);
    BOOST_TEST(max != three_quarter_max);
    BOOST_TEST(half_max != quarter_max);
    BOOST_TEST(half_max != three_quarter_max);
    BOOST_TEST(quarter_max != three_quarter_max);

    BOOST_TEST(max != -max);
    BOOST_TEST(half_max != -half_max);
    BOOST_TEST(quarter_max != -quarter_max);
    BOOST_TEST(three_quarter_max != -three_quarter_max);


#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(max) == BOOST_HASH_TEST_NAMESPACE::hash_value(max));
    BOOST_TEST(x1(half_max) == BOOST_HASH_TEST_NAMESPACE::hash_value(half_max));
    BOOST_TEST(x1(quarter_max) == BOOST_HASH_TEST_NAMESPACE::hash_value(quarter_max));
    BOOST_TEST(x1(three_quarter_max) ==
        BOOST_HASH_TEST_NAMESPACE::hash_value(three_quarter_max));
#endif

    // The '!=' tests could legitimately fail, but with my hash it indicates a
    // bug.
    BOOST_TEST(x1(max) == x1(max));
    BOOST_TEST(x1(max) != x1(quarter_max));
    BOOST_TEST(x1(max) != x1(half_max));
    BOOST_TEST(x1(max) != x1(three_quarter_max));
    BOOST_TEST(x1(quarter_max) == x1(quarter_max));
    BOOST_TEST(x1(quarter_max) != x1(half_max));
    BOOST_TEST(x1(quarter_max) != x1(three_quarter_max));
    BOOST_TEST(x1(half_max) == x1(half_max));
    BOOST_TEST(x1(half_max) != x1(three_quarter_max));
    BOOST_TEST(x1(three_quarter_max) == x1(three_quarter_max));

    BOOST_TEST(x1(max) != x1(-max));
    BOOST_TEST(x1(half_max) != x1(-half_max));
    BOOST_TEST(x1(quarter_max) != x1(-quarter_max));
    BOOST_TEST(x1(three_quarter_max) != x1(-three_quarter_max));


// Intel with gcc stdlib sometimes segfaults on calls to asin and acos.
#if !((defined(__INTEL_COMPILER) || defined(__ICL) || \
        defined(__ICC) || defined(__ECC)) && \
    (defined(__GLIBCPP__) || defined(__GLIBCXX__)))
    T v1 = asin((T) 1);
    T v2 = acos((T) 0);
    if(v1 == v2)
        BOOST_TEST(x1(v1) == x1(v2));

#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(v1) == BOOST_HASH_TEST_NAMESPACE::hash_value(v1));
    BOOST_TEST(x1(v2) == BOOST_HASH_TEST_NAMESPACE::hash_value(v2));
#endif
    
#endif

#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(boost::hash_detail::limits<T>::epsilon()) ==
            BOOST_HASH_TEST_NAMESPACE::hash_value(
                boost::hash_detail::limits<T>::epsilon()));
#endif

    BOOST_TEST(boost::hash_detail::limits<T>::epsilon() != (T) 0);
    if(x1(boost::hash_detail::limits<T>::epsilon()) == x1((T) 0))
        std::cerr<<"x1(epsilon) == x1(0) == "<<x1((T) 0)<<"\n";

    BOOST_TEST(-boost::hash_detail::limits<T>::epsilon() != (T) 0);
    if(x1(-boost::hash_detail::limits<T>::epsilon()) == x1((T) 0))
        std::cerr<<"x1(-epsilon) == x1(0) == "<<x1((T) 0)<<"\n";

    BOOST_TEST((T) 1 + boost::hash_detail::limits<T>::epsilon() != (T) 1);
    if(x1((T) 1 + boost::hash_detail::limits<T>::epsilon()) == x1((T) 1))
        std::cerr<<"x1(1 + epsilon) == x1(1) == "<<x1((T) 1)<<"\n";

    BOOST_TEST((T) 1 - boost::hash_detail::limits<T>::epsilon() != (T) 1);
    if(x1((T) 1 - boost::hash_detail::limits<T>::epsilon()) == x1((T) 1))
        std::cerr<<"x1(1 - epsilon) == x1(1) == "<<x1((T) 1)<<"\n";

    BOOST_TEST((T) -1 + boost::hash_detail::limits<T>::epsilon() != (T) -1);
    if(x1((T) -1 + boost::hash_detail::limits<T>::epsilon()) == x1((T) -1))
        std::cerr<<"x1(-1 + epsilon) == x1(-1) == "<<x1((T) -1)<<"\n";

    BOOST_TEST((T) -1 - boost::hash_detail::limits<T>::epsilon() != (T) -1);
    if(x1((T) -1 - boost::hash_detail::limits<T>::epsilon()) == x1((T) -1))
        std::cerr<<"x1(-1 - epsilon) == x1(-1) == "<<x1((T) -1)<<"\n";

    // As before.
    if(boost::hash_detail::limits<T>::has_denorm) {
        if(x1(boost::hash_detail::limits<T>::denorm_min()) == x1(zero)) {
            std::cerr<<"x1(denorm_min) == x1(zero) == "<<x1(zero)<<"\n";
        }
#if !BOOST_WORKAROUND(__DECCXX_VER,<70190006) && defined(BOOST_HASH_TEST_EXTENSIONS)
        // The Tru64/CXX standard library prior to 7.1 contains a bug in the
        // specialization of boost::hash_detail::limits::denorm_min() for long
        // doubles which causes this test to fail.
        if(x1(boost::hash_detail::limits<T>::denorm_min()) !=
            BOOST_HASH_TEST_NAMESPACE::hash_value(
                boost::hash_detail::limits<T>::denorm_min()))
        {
            std::cerr
                <<  "x1(boost::hash_detail::limits<T>::denorm_min()) = "
                <<  x1(boost::hash_detail::limits<T>::denorm_min())
                <<  "\nhash_value(boost::hash_detail::limits<T>::denorm_min())"
                    " = "
                <<  BOOST_HASH_TEST_NAMESPACE::hash_value(
                        boost::hash_detail::limits<T>::denorm_min())
                <<  "\nx1(0) = "
                <<  x1(0)
                <<  "\n";
        }
#endif
    }

// NaN also causes borland to crash.
#if !defined(__BORLANDC__) && defined(BOOST_HASH_TEST_EXTENSIONS)
    if(boost::hash_detail::limits<T>::has_quiet_NaN) {
        if(x1(boost::hash_detail::limits<T>::quiet_NaN()) == x1(1.0)) {
            std::cerr<<"x1(quiet_NaN) == x1(1.0) == "<<x1(1.0)<<"\n";
        }
        BOOST_TEST(x1(boost::hash_detail::limits<T>::quiet_NaN()) ==
            BOOST_HASH_TEST_NAMESPACE::hash_value(
                boost::hash_detail::limits<T>::quiet_NaN()));
    }
#endif
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif
