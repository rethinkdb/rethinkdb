// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/static_assert.hpp>
#include <boost/python/detail/if_else.hpp>
#include <boost/type_traits/same_traits.hpp>

    typedef char c1;
    typedef char c2[2];
    typedef char c3[3];
    typedef char c4[4];

template <unsigned size>
struct choose
{
    typedef typename boost::python::detail::if_<
        (sizeof(c1) == size)
    >::template then<
        c1
    >::template elif<
        (sizeof(c2) == size)
    >::template then<
        c2
    >::template elif<
        (sizeof(c3) == size)
    >::template then<
        c3
    >::template elif<
        (sizeof(c4) == size)
    >::template then<
        c4
    >::template else_<void*>::type type;
};

int main()
{
    BOOST_STATIC_ASSERT((boost::is_same<choose<1>::type,c1>::value));
    BOOST_STATIC_ASSERT((boost::is_same<choose<2>::type,c2>::value));
    BOOST_STATIC_ASSERT((boost::is_same<choose<3>::type,c3>::value));
    BOOST_STATIC_ASSERT((boost::is_same<choose<4>::type,c4>::value));
    BOOST_STATIC_ASSERT((boost::is_same<choose<5>::type,void*>::value));
    return 0;
}
