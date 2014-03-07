
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#if defined(BOOST_HASH_TEST_EXTENSIONS) && !defined(BOOST_HASH_TEST_STD_INCLUDES)
#include <boost/functional/hash_fwd.hpp>

#include <boost/config.hpp>
#include <cstddef>
#include <vector>

namespace test {

    template <class T>
    struct test_type1
    {
        T value;
        test_type1(T const& x) : value(x) {}
    };

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
    template <class T>
    std::size_t hash_value(test_type1<T> const& x)
    {
        BOOST_HASH_TEST_NAMESPACE::hash<T> hasher;
        return hasher(x.value);
    }
#endif

    template <class T>
    struct test_type2
    {
        T value1, value2;
        test_type2(T const& x, T const& y) : value1(x), value2(y) {}
    };

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
    template <class T>
    std::size_t hash_value(test_type2<T> const& x)
    {
        std::size_t seed = 0;
        BOOST_HASH_TEST_NAMESPACE::hash_combine(seed, x.value1);
        BOOST_HASH_TEST_NAMESPACE::hash_combine(seed, x.value2);
        return seed;
    }
#endif

    template <class T>
    struct test_type3
    {
        std::vector<T> values;
        test_type3(typename std::vector<T>::iterator x,
                typename std::vector<T>::iterator y) : values(x, y) {}
    };

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
    template <class T>
    std::size_t hash_value(test_type3<T> const& x)
    {
        std::size_t seed =
            BOOST_HASH_TEST_NAMESPACE::hash_range(x.values.begin(), x.values.end());
        BOOST_HASH_TEST_NAMESPACE::hash_range(seed, x.values.begin(), x.values.end());
        return seed;
    }
#endif

}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)

namespace boost
{
    template <class T>
    std::size_t hash_value(test::test_type1<T> const& x)
    {
        BOOST_HASH_TEST_NAMESPACE::hash<T> hasher;
        return hasher(x.value);
    }

    template <class T>
    std::size_t hash_value(test::test_type2<T> const& x)
    {
        std::size_t seed = 0;
        BOOST_HASH_TEST_NAMESPACE::hash_combine(seed, x.value1);
        BOOST_HASH_TEST_NAMESPACE::hash_combine(seed, x.value2);
        return seed;
    }

    template <class T>
    std::size_t hash_value(test::test_type3<T> const& x)
    {
        std::size_t seed =
            BOOST_HASH_TEST_NAMESPACE::hash_range(x.values.begin(), x.values.end());
        BOOST_HASH_TEST_NAMESPACE::hash_range(seed, x.values.begin(), x.values.end());
        return seed;
    }
}

#endif

#endif
