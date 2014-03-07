// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef STATIC_ASSERT_SAME_DWA2003530_HPP
# define STATIC_ASSERT_SAME_DWA2003530_HPP

#include <boost/mpl/assert.hpp>
# include <boost/type_traits/is_same.hpp>

#define STATIC_ASSERT_SAME( T1,T2 ) BOOST_MPL_ASSERT((::boost::is_same< T1, T2 >))

template <class T1, class T2>
struct static_assert_same
{
    BOOST_MPL_ASSERT((::boost::is_same< T1, T2 >));
    enum { value = 1 };
};

#endif // STATIC_ASSERT_SAME_DWA2003530_HPP
