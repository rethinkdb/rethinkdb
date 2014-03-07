
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_HELPERS_CHECK_RETURN_TYPE_HEADER)
#define BOOST_UNORDERED_TEST_HELPERS_CHECK_RETURN_TYPE_HEADER

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace test
{
    template <class T1>
    struct check_return_type
    {
        template <class T2>
        static void equals(T2)
        {
            BOOST_STATIC_ASSERT((boost::is_same<T1, T2>::value));
        }

        template <class T2>
        static void equals_ref(T2&)
        {
            BOOST_STATIC_ASSERT((boost::is_same<T1, T2>::value));
        }

        template <class T2>
        static void convertible(T2)
        {
            BOOST_STATIC_ASSERT((boost::is_convertible<T2, T1>::value));
        }
    };
}

#endif
