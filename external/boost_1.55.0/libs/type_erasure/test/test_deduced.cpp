// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_deduced.cpp 78430 2012-05-12 03:56:07Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/deduced.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_same.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

BOOST_AUTO_TEST_CASE(test_deduce_dereference)
{
    typedef ::boost::mpl::vector<
        copy_constructible<>,
        typeid_<deduced<boost::remove_pointer<_self> >::type>,
        dereferenceable<deduced<boost::remove_pointer<_self> >&>
    > test_concept;
    int i;
    any<test_concept> x(&i);
    any<test_concept, deduced<boost::remove_pointer<_self> >&> y(*x);
    BOOST_CHECK_EQUAL(&any_cast<int&>(y), &i);
}

BOOST_MPL_ASSERT((
    boost::is_same<
        deduced<boost::remove_pointer<_self> >::type,
        deduced<boost::remove_pointer<_self> > >));

BOOST_MPL_ASSERT((
    boost::is_same<deduced<boost::remove_pointer<int*> >::type, int >));
