// Copyright 2005 Alexander Nasonov.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef FILE_boost_libs_type_traits_test_promote_util_hpp_INCLUDED
#define FILE_boost_libs_type_traits_test_promote_util_hpp_INCLUDED

#include <boost/type_traits/promote.hpp>

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

template<class T, class Promoted>
inline void test_no_cv()
{
    typedef BOOST_DEDUCED_TYPENAME boost::promote<T>::type promoted;
    BOOST_STATIC_ASSERT((  boost::is_same<promoted,Promoted>::value ));
}

template<class T, class Promoted>
inline void test_cv()
{
    typedef BOOST_DEDUCED_TYPENAME boost::promote<T               >::type promoted;
    typedef BOOST_DEDUCED_TYPENAME boost::promote<T const         >::type promoted_c;
    typedef BOOST_DEDUCED_TYPENAME boost::promote<T       volatile>::type promoted_v;
    typedef BOOST_DEDUCED_TYPENAME boost::promote<T const volatile>::type promoted_cv;

    BOOST_STATIC_ASSERT(( ::boost::is_same< promoted   , Promoted                >::value ));
    BOOST_STATIC_ASSERT(( ::boost::is_same< promoted_c , Promoted const          >::value ));
    BOOST_STATIC_ASSERT(( ::boost::is_same< promoted_v , Promoted       volatile >::value ));
    BOOST_STATIC_ASSERT(( ::boost::is_same< promoted_cv, Promoted const volatile >::value ));
}

#endif // #ifndef FILE_boost_libs_type_traits_test_promote_util_hpp_INCLUDED

