// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_same_type.cpp 78459 2012-05-13 18:20:31Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/deduced.hpp>
#include <boost/type_erasure/same_type.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_same.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

BOOST_AUTO_TEST_CASE(test_deduce_dereference)
{
    typedef ::boost::mpl::vector<
        copy_constructible<>,
        typeid_<_a>,
        dereferenceable<deduced<boost::remove_pointer<_self> >&>,
        same_type<deduced<boost::remove_pointer<_self> >, _a>
    > test_concept;
    int i;

    any<test_concept> x(&i);
    any<test_concept, _a&> y(*x);
    BOOST_CHECK_EQUAL(&any_cast<int&>(y), &i);
}

BOOST_MPL_ASSERT((
    boost::is_same<
        deduced<boost::remove_pointer<_self> >::type,
        deduced<boost::remove_pointer<_self> > >));

BOOST_MPL_ASSERT((
    boost::is_same<deduced<boost::remove_pointer<int*> >::type, int >));

BOOST_AUTO_TEST_CASE(test_duplicate)
{
    typedef ::boost::mpl::vector<
        copy_constructible<>,
        typeid_<_a>,
        dereferenceable<deduced<boost::remove_pointer<_self> >&>,
        same_type<deduced<boost::remove_pointer<_self> >, _a>,
        same_type<deduced<boost::remove_pointer<_self> >, _a>
    > test_concept;
    int i;

    any<test_concept> x(&i);
    any<test_concept, _a&> y(*x);
    BOOST_CHECK_EQUAL(&any_cast<int&>(y), &i);
}

BOOST_AUTO_TEST_CASE(test_convert)
{
    typedef ::boost::mpl::vector<
        copy_constructible<>,
        typeid_<_a>,
        dereferenceable<deduced<boost::remove_pointer<_self> >&>,
        same_type<deduced<boost::remove_pointer<_self> >, _a>
    > test_concept_src;
    typedef ::boost::mpl::vector<
        copy_constructible<_b>,
        typeid_<_c>,
        dereferenceable<deduced<boost::remove_pointer<_b> >&, _b>,
        same_type<deduced<boost::remove_pointer<_b> >, _c>
    > test_concept_dest;
    int i;

    any<test_concept_src> x1(&i);
    any<test_concept_src, _a&> y1(*x1);
    any<test_concept_dest, _b> x2(x1);
    any<test_concept_dest, _c&> y2(*x2);
    BOOST_CHECK_EQUAL(&any_cast<int&>(y2), &i);
}
