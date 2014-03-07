// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/bases.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/same_traits.hpp>

struct A;
struct B;

template <class X, class Y, class Z>
struct choose_bases
    : boost::python::detail::select_bases<
    X
    , typename boost::python::detail::select_bases<
        Y
        , typename boost::python::detail::select_bases<Z>::type
    >::type>
{
    
};

int main()
{
    BOOST_STATIC_ASSERT((boost::python::detail::specifies_bases<
                         boost::python::bases<A,B> >::value));

    BOOST_STATIC_ASSERT((!boost::python::detail::specifies_bases<
                         boost::python::bases<A,B>& >::value));

    BOOST_STATIC_ASSERT((!boost::python::detail::specifies_bases<
                         void* >::value));

    BOOST_STATIC_ASSERT((!boost::python::detail::specifies_bases<
                         int >::value));

    BOOST_STATIC_ASSERT((!boost::python::detail::specifies_bases<
                         int[5] >::value));

    typedef boost::python::detail::select_bases<
        int
        , boost::python::detail::select_bases<char*>::type > collected1;

    BOOST_STATIC_ASSERT((boost::is_same<collected1::type,boost::python::bases<> >::value));
    BOOST_STATIC_ASSERT((boost::is_same<choose_bases<int,char*,long>::type,boost::python::bases<> >::value));
    
    typedef boost::python::detail::select_bases<
        int
        , boost::python::detail::select_bases<
                boost::python::bases<A,B>
                , boost::python::detail::select_bases<
                        A
            >::type
         >::type
     > collected2;

    BOOST_STATIC_ASSERT((boost::is_same<collected2::type,boost::python::bases<A,B> >::value));
    BOOST_STATIC_ASSERT((boost::is_same<choose_bases<int,boost::python::bases<A,B>,long>::type,boost::python::bases<A,B> >::value));
    
    return 0;
}
