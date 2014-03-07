
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif

#include <iostream>
#include <boost/detail/lightweight_test.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/functional/hash/detail/limits.hpp>
#include <boost/utility/enable_if.hpp>

#include "./compile_time.hpp"

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4309) // truncation of constant value
#pragma warning(disable:4310) // cast truncates constant value
#endif

#if defined(__GNUC__) && !defined(BOOST_INTEL_CXX_VERSION)
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

template <class T>
void numeric_extra_tests(typename
    boost::enable_if_c<boost::hash_detail::limits<T>::is_integer,
        void*>::type = 0)
{
    typedef boost::hash_detail::limits<T> limits;

    if(limits::is_signed ||
        limits::digits <= boost::hash_detail::limits<std::size_t>::digits)
    {
        BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(T(-5)) == (std::size_t)T(-5));
    }
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(T(0)) == (std::size_t)T(0u));
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(T(10)) == (std::size_t)T(10u));
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(T(25)) == (std::size_t)T(25u));
}

template <class T>
void numeric_extra_tests(typename
    boost::disable_if_c<boost::hash_detail::limits<T>::is_integer,
        void*>::type = 0)
{
}

template <class T>
void numeric_test(T*)
{
    typedef boost::hash_detail::limits<T> limits;

    compile_time_tests((T*) 0);

    BOOST_HASH_TEST_NAMESPACE::hash<T> x1;
    BOOST_HASH_TEST_NAMESPACE::hash<T> x2;

    T v1 = (T) -5;
    BOOST_TEST(x1(v1) == x2(v1));
    BOOST_TEST(x1(T(-5)) == x2(T(-5)));
    BOOST_TEST(x1(T(0)) == x2(T(0)));
    BOOST_TEST(x1(T(10)) == x2(T(10)));
    BOOST_TEST(x1(T(25)) == x2(T(25)));
    BOOST_TEST(x1(T(5) - T(5)) == x2(T(0)));
    BOOST_TEST(x1(T(6) + T(4)) == x2(T(10)));

#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(T(-5)) == BOOST_HASH_TEST_NAMESPACE::hash_value(T(-5)));
    BOOST_TEST(x1(T(0)) == BOOST_HASH_TEST_NAMESPACE::hash_value(T(0)));
    BOOST_TEST(x1(T(10)) == BOOST_HASH_TEST_NAMESPACE::hash_value(T(10)));
    BOOST_TEST(x1(T(25)) == BOOST_HASH_TEST_NAMESPACE::hash_value(T(25)));

    numeric_extra_tests<T>();
#endif
}

template <class T>
void limits_test(T*)
{
    typedef boost::hash_detail::limits<T> limits;

    if(limits::is_specialized)
    {
        BOOST_HASH_TEST_NAMESPACE::hash<T> x1;
        BOOST_HASH_TEST_NAMESPACE::hash<T> x2;

        T min_value = (limits::min)();
        T max_value = (limits::max)();

        BOOST_TEST(x1(min_value) == x2((limits::min)()));
        BOOST_TEST(x1(max_value) == x2((limits::max)()));

#if defined(BOOST_HASH_TEST_EXTENSIONS)
        BOOST_TEST(x1(min_value) == BOOST_HASH_TEST_NAMESPACE::hash_value(min_value));
        BOOST_TEST(x1(max_value) == BOOST_HASH_TEST_NAMESPACE::hash_value(max_value));

        if (limits::is_integer)
        {
            BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(min_value)
                    == std::size_t(min_value));
            BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(max_value)
                    == std::size_t(max_value));
        }
#endif
    }
}

template <class T>
void poor_quality_tests(T*)
{
    typedef boost::hash_detail::limits<T> limits;

    BOOST_HASH_TEST_NAMESPACE::hash<T> x1;
    BOOST_HASH_TEST_NAMESPACE::hash<T> x2;

    // A hash function can legally fail these tests, but it'll not be a good
    // sign.
    if(T(1) != T(-1))
        BOOST_TEST(x1(T(1)) !=  x2(T(-1)));
    if(T(1) != T(2))
        BOOST_TEST(x1(T(1)) !=  x2(T(2)));
    if((limits::max)() != (limits::max)() - 1)
        BOOST_TEST(x1(static_cast<T>((limits::max)()))
            != x2(static_cast<T>((limits::max)() - 1)));
}

void bool_test()
{
    BOOST_HASH_TEST_NAMESPACE::hash<bool> x1;
    BOOST_HASH_TEST_NAMESPACE::hash<bool> x2;
    
    BOOST_TEST(x1(true) == x2(true));
    BOOST_TEST(x1(false) == x2(false));
    BOOST_TEST(x1(true) != x2(false));
    BOOST_TEST(x1(false) != x2(true));
}

#define NUMERIC_TEST(type, name) \
    std::cerr<<"Testing: " BOOST_STRINGIZE(name) "\n"; \
    numeric_test((type*) 0); \
    limits_test((type*) 0); \
    poor_quality_tests((type*) 0);
#define NUMERIC_TEST_NO_LIMITS(type, name) \
    std::cerr<<"Testing: " BOOST_STRINGIZE(name) "\n"; \
    numeric_test((type*) 0); \
    poor_quality_tests((type*) 0);

int main()
{
    NUMERIC_TEST(char, char)
    NUMERIC_TEST(signed char, schar)
    NUMERIC_TEST(unsigned char, uchar)
#ifndef BOOST_NO_INTRINSIC_WCHAR_T
    NUMERIC_TEST(wchar_t, wchar)
#endif
    NUMERIC_TEST(short, short)
    NUMERIC_TEST(unsigned short, ushort)
    NUMERIC_TEST(int, int)
    NUMERIC_TEST(unsigned int, uint)
    NUMERIC_TEST(long, hash_long)
    NUMERIC_TEST(unsigned long, ulong)

#if !defined(BOOST_NO_LONG_LONG)
    NUMERIC_TEST_NO_LIMITS(boost::long_long_type, long_long)
    NUMERIC_TEST_NO_LIMITS(boost::ulong_long_type, ulong_long)
#endif

#if defined(BOOST_HAS_INT128)
    NUMERIC_TEST_NO_LIMITS(boost::int128_type, int128)
    NUMERIC_TEST_NO_LIMITS(boost::uint128_type, uint128)
#endif

    NUMERIC_TEST(float, float)
    NUMERIC_TEST(double, double)

    NUMERIC_TEST(std::size_t, size_t)
    NUMERIC_TEST(std::ptrdiff_t, ptrdiff_t)

    bool_test();

    return boost::report_errors();
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif
