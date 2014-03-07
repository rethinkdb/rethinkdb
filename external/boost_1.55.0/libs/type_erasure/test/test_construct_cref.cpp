// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_construct_cref.cpp 83251 2013-03-02 19:23:44Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/mpl/vector.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

template<class T>
T as_rvalue(const T& arg) { return arg; }
template<class T>
const T& as_const(const T& arg) { return arg; }

BOOST_AUTO_TEST_CASE(test_implicit) {
    int i = 4;
    any<common<>, const _self&> x = i;
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
}

BOOST_AUTO_TEST_CASE(test_from_int_with_binding)
{
    typedef ::boost::mpl::vector<common<> > test_concept;
    int i = 4;
    any<test_concept, const _self&> x(i, make_binding<boost::mpl::map<boost::mpl::pair<_self, int> > >());
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
}

BOOST_AUTO_TEST_CASE(test_copy)
{
    typedef ::boost::mpl::vector<typeid_<> > test_concept;
    int i = 4;
    any<test_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<test_concept, const _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<test_concept, const _self&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<test_concept, const _self&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_convert)
{
    typedef ::boost::mpl::vector<common<>, typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<> > dst_concept;
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _self&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<dst_concept, const _self&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_rebind)
{
    typedef ::boost::mpl::vector<typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<_a> > dst_concept;
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _a&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _a&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<dst_concept, const _a&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert)
{
    typedef ::boost::mpl::vector<common<>, typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<_a> > dst_concept;
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _a&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _a&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<dst_concept, const _a&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_with_binding)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, _self> > map;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, int> > types;

    binding<dst_concept> table(make_binding<types>());
    
    int i = 4;
    any<src_concept, const _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _a&> y(x, make_binding<map>());
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _a&> z(x, table);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    
    any<dst_concept, const _a&> cy(as_const(x), make_binding<map>());
    BOOST_CHECK_EQUAL(any_cast<const int*>(&cy), &i);
    any<dst_concept, const _a&> cz(as_const(x), table);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&cz), &i);
}

BOOST_AUTO_TEST_CASE(test_copy_from_ref)
{
    typedef ::boost::mpl::vector<typeid_<> > test_concept;
    int i = 4;
    any<test_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<test_concept, const _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<test_concept, const _self&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<test_concept, const _self&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_convert_from_ref)
{
    typedef ::boost::mpl::vector<common<>, typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<> > dst_concept;
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _self&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<dst_concept, const _self&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_rebind_from_ref)
{
    typedef ::boost::mpl::vector<typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<_a> > dst_concept;
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _a&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _a&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<dst_concept, const _a&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_from_ref)
{
    typedef ::boost::mpl::vector<common<>, typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<_a> > dst_concept;
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _a&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _a&> z = as_rvalue(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    any<dst_concept, const _a&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), &i);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_with_binding_from_ref)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, _self> > map;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, int> > types;

    binding<dst_concept> table(make_binding<types>());
    
    int i = 4;
    any<src_concept, _self&> x(i);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&x), &i);
    any<dst_concept, const _a&> y(x, make_binding<map>());
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), &i);
    any<dst_concept, const _a&> z(x, table);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), &i);
    
    any<dst_concept, const _a&> cy(as_const(x), make_binding<map>());
    BOOST_CHECK_EQUAL(any_cast<const int*>(&cy), &i);
    any<dst_concept, const _a&> cz(as_const(x), table);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&cz), &i);
}

BOOST_AUTO_TEST_CASE(test_copy_from_value)
{
    typedef ::boost::mpl::vector<copy_constructible<>, typeid_<> > test_concept;
    any<test_concept> x(4);
    const int* ip = any_cast<const int*>(&x);
    any<test_concept, const _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), ip);
    BOOST_CHECK_EQUAL(any_cast<int>(any<test_concept, const _self&>(as_rvalue(x))), 4);
    any<test_concept, const _self&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), ip);
}

BOOST_AUTO_TEST_CASE(test_convert_from_value)
{
    typedef ::boost::mpl::vector<common<>, typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<> > dst_concept;
    any<src_concept> x(4);
    const int* ip = any_cast<const int*>(&x);
    any<dst_concept, const _self&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), ip);
    BOOST_CHECK_EQUAL(any_cast<int>(any<dst_concept, const _self&>(as_rvalue(x))), 4);
    any<dst_concept, const _self&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), ip);
}

BOOST_AUTO_TEST_CASE(test_rebind_from_value)
{
    typedef ::boost::mpl::vector<copy_constructible<>, typeid_<> > src_concept;
    typedef ::boost::mpl::vector<copy_constructible<_a>, typeid_<_a> > dst_concept;
    any<src_concept> x(4);
    const int* ip = any_cast<const int*>(&x);
    any<dst_concept, const _a&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), ip);
    BOOST_CHECK_EQUAL(any_cast<int>(any<dst_concept, const _a&>(as_rvalue(x))), 4);
    any<dst_concept, const _a&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), ip);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_from_value)
{
    typedef ::boost::mpl::vector<common<>, typeid_<> > src_concept;
    typedef ::boost::mpl::vector<typeid_<_a> > dst_concept;
    any<src_concept> x(4);
    const int* ip = any_cast<const int*>(&x);
    any<dst_concept, const _a&> y(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), ip);
    BOOST_CHECK_EQUAL(any_cast<int>(any<dst_concept, const _a&>(as_rvalue(x))), 4);
    any<dst_concept, const _a&> w = as_const(x);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&w), ip);
}

BOOST_AUTO_TEST_CASE(test_rebind_and_convert_with_binding_from_value)
{
    typedef ::boost::mpl::vector<common<>, incrementable<> > src_concept;
    typedef ::boost::mpl::vector<common<_a> > dst_concept;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, _self> > map;
    typedef ::boost::mpl::map<boost::mpl::pair<_a, int> > types;

    binding<dst_concept> table(make_binding<types>());
    
    any<src_concept> x(4);
    const int* ip = any_cast<const int*>(&x);
    any<dst_concept, const _a&> y(x, make_binding<map>());
    BOOST_CHECK_EQUAL(any_cast<const int*>(&y), ip);
    any<dst_concept, const _a&> z(x, table);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&z), ip);
    
    any<dst_concept, const _a&> cy(as_const(x), make_binding<map>());
    BOOST_CHECK_EQUAL(any_cast<const int*>(&cy), ip);
    any<dst_concept, const _a&> cz(as_const(x), table);
    BOOST_CHECK_EQUAL(any_cast<const int*>(&cz), ip);
}
