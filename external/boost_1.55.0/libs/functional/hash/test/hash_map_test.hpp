
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(CONTAINER_TYPE)
#error "CONTAINER_TYPE not defined"
#else

#include <boost/preprocessor/cat.hpp>

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4244) // conversion from 'int' to 'float'
#pragma warning(disable:4245) // signed/unsigned mismatch
#endif

namespace BOOST_PP_CAT(CONTAINER_TYPE, _tests)
{
    template <class T>
    void integer_tests(T* = 0)
    {
        const int number_of_containers = 10;
        T containers[number_of_containers];
        typedef BOOST_DEDUCED_TYPENAME T::value_type pair;
        typedef BOOST_DEDUCED_TYPENAME T::key_type key;
        typedef BOOST_DEDUCED_TYPENAME T::mapped_type value;

        for(int i = 0; i < 5; ++i) {
            for(int j = 0; j < i; ++j)
                containers[i].insert(pair(key(0), value(0)));
        }

        containers[6].insert(pair(key(1),value(0)));
        containers[7].insert(pair(key(1),value(0)));
        containers[7].insert(pair(key(1),value(0)));
        containers[8].insert(pair(key(-1),value(1)));
        containers[9].insert(pair(key(-1),value(3)));
        containers[9].insert(pair(key(-1),value(3)));

        BOOST_HASH_TEST_NAMESPACE::hash<T> hasher;

        for(int i2 = 0; i2 < number_of_containers; ++i2) {
            BOOST_TEST(hasher(containers[i2]) == hasher(containers[i2]));

            BOOST_TEST(hasher(containers[i2]) ==
                    BOOST_HASH_TEST_NAMESPACE::hash_value(containers[i2]));

            BOOST_TEST(hasher(containers[i2])
                    == BOOST_HASH_TEST_NAMESPACE::hash_range(
                        containers[i2].begin(), containers[i2].end()));

            for(int j2 = i2 + 1; j2 < number_of_containers; ++j2) {
                BOOST_TEST(
                        (containers[i2] == containers[j2]) ==
                        (hasher(containers[i2]) == hasher(containers[j2]))
                        );
            }
        }
    }

    void BOOST_PP_CAT(CONTAINER_TYPE, _hash_integer_tests())
    {
        integer_tests((CONTAINER_TYPE<char, unsigned char>*) 0);
        integer_tests((CONTAINER_TYPE<int, float>*) 0);
        integer_tests((CONTAINER_TYPE<unsigned long, unsigned long>*) 0);
        integer_tests((CONTAINER_TYPE<double, short>*) 0);
    }
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#undef CONTAINER_TYPE
#endif
