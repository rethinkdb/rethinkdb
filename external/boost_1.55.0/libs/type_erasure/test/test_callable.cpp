// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_callable.cpp 83325 2013-03-05 22:24:25Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/callable.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/tuple.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/result_of.hpp>
#include <vector>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

template<class T = _self>
struct common : ::boost::mpl::vector<
    copy_constructible<T>,
    typeid_<T>
> {};

int f1_val;
void f1() { ++f1_val; }

int f2_val;
int f2() { return ++f2_val; }

BOOST_AUTO_TEST_CASE(test_void)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<void()>
    > test_concept;
    any<test_concept> x1(&f1);
    f1_val = 0;
    x1();
    BOOST_CHECK_EQUAL(f1_val, 1);

    any<test_concept> x2(&f2);
    f2_val = 0;
    x2();
    BOOST_CHECK_EQUAL(f2_val, 1);
    
    typedef ::boost::mpl::vector<
        common<>,
        callable<int()>
    > test_concept_int;
    any<test_concept_int> x3(&f2);
    f2_val = 0;
    int i = x3();
    BOOST_CHECK_EQUAL(i, 1);
    BOOST_CHECK_EQUAL(f2_val, 1);
}

BOOST_AUTO_TEST_CASE(test_void_const)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<void(), const _self>
    > test_concept;
    const any<test_concept> x1(&f1);
    f1_val = 0;
    x1();
    BOOST_CHECK_EQUAL(f1_val, 1);

    const any<test_concept> x2(&f2);
    f2_val = 0;
    x2();
    BOOST_CHECK_EQUAL(f2_val, 1);
    
    typedef ::boost::mpl::vector<
        common<>,
        callable<int(), const _self>
    > test_concept_int;
    const any<test_concept_int> x3(&f2);
    f2_val = 0;
    int i = x3();
    BOOST_CHECK_EQUAL(i, 1);
    BOOST_CHECK_EQUAL(f2_val, 1);
}

int f3_val;
void f3(int i) { f3_val += i; }

int f4_val;
int f4(int i) { return f4_val += i; }

BOOST_AUTO_TEST_CASE(test_int)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<void(int)>
    > test_concept;
    any<test_concept> x1(&f3);
    f3_val = 1;
    x1(3);
    BOOST_CHECK_EQUAL(f3_val, 4);

    any<test_concept> x2(&f4);
    f4_val = 1;
    x2(2);
    BOOST_CHECK_EQUAL(f4_val, 3);
    
    typedef ::boost::mpl::vector<
        common<>,
        callable<int(int)>
    > test_concept_int;
    any<test_concept_int> x3(&f4);
    f4_val = 1;
    int i = x3(4);
    BOOST_CHECK_EQUAL(i, 5);
    BOOST_CHECK_EQUAL(f4_val, 5);
}

BOOST_AUTO_TEST_CASE(test_int_const)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<void(int), const _self>
    > test_concept;
    const any<test_concept> x1(&f3);
    f3_val = 1;
    x1(3);
    BOOST_CHECK_EQUAL(f3_val, 4);

    const any<test_concept> x2(&f4);
    f4_val = 1;
    x2(2);
    BOOST_CHECK_EQUAL(f4_val, 3);
    
    typedef ::boost::mpl::vector<
        common<>,
        callable<int(int), const _self>
    > test_concept_int;
    const any<test_concept_int> x3(&f4);
    f4_val = 1;
    int i = x3(4);
    BOOST_CHECK_EQUAL(i, 5);
    BOOST_CHECK_EQUAL(f4_val, 5);
}

BOOST_AUTO_TEST_CASE(test_any)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void(_a)>
    > test_concept;
    tuple<test_concept, _self, _a> t1(&f3, 3);
    any<test_concept> x1(get<0>(t1));
    f3_val = 1;
    x1(get<1>(t1));
    BOOST_CHECK_EQUAL(f3_val, 4);
    
    tuple<test_concept, _self, _a> t2(&f4, 2);
    any<test_concept> x2(get<0>(t2));
    f4_val = 1;
    x2(get<1>(t2));
    BOOST_CHECK_EQUAL(f4_val, 3);
    
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<_a(_a)>
    > test_concept_int;
    tuple<test_concept_int, _self, _a> t3(&f4, 4);
    any<test_concept_int> x3(get<0>(t3));
    f4_val = 1;
    int i = any_cast<int>(x3(get<1>(t3)));
    BOOST_CHECK_EQUAL(i, 5);
    BOOST_CHECK_EQUAL(f4_val, 5);
}

BOOST_AUTO_TEST_CASE(test_any_const)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void(_a), const _self>
    > test_concept;
    tuple<test_concept, _self, _a> t1(&f3, 3);
    const any<test_concept> x1(get<0>(t1));
    f3_val = 1;
    x1(get<1>(t1));
    BOOST_CHECK_EQUAL(f3_val, 4);
    
    tuple<test_concept, _self, _a> t2(&f4, 2);
    const any<test_concept> x2(get<0>(t2));
    f4_val = 1;
    x2(get<1>(t2));
    BOOST_CHECK_EQUAL(f4_val, 3);
    
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<_a(_a), const _self>
    > test_concept_int;
    tuple<test_concept_int, _self, _a> t3(&f4, 4);
    const any<test_concept_int> x3(get<0>(t3));
    f4_val = 1;
    int i = any_cast<int>(x3(get<1>(t3)));
    BOOST_CHECK_EQUAL(i, 5);
    BOOST_CHECK_EQUAL(f4_val, 5);
}

int overload1;
int overload2;
int overload3;

struct overloaded_function
{
    int operator()() const { return ++overload1; }
    int operator()(int i) const { return overload2 += i; }
    int operator()(short i) const { return overload3 += i; }
};

BOOST_AUTO_TEST_CASE(test_result_of)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void()>,
        callable<int(int)>,
        callable<long(_a)>
    > test_concept;
    
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>()>::type, void>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>(int)>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>(any<test_concept, _a>)>::type, long>));
}

BOOST_AUTO_TEST_CASE(test_result_of_const)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void(), const _self>,
        callable<int(int), const _self>,
        callable<long(_a), const _self>
    > test_concept;
    
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>()>::type, void>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>(int)>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>(any<test_concept, _a>)>::type, long>));
}

BOOST_AUTO_TEST_CASE(test_overload)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void()>,
        callable<void(int)>,
        callable<void(_a)>
    > test_concept;
    tuple<test_concept, _self, _a> t(overloaded_function(), static_cast<short>(3));
    any<test_concept> f(get<0>(t));
    any<test_concept, _a> a(get<1>(t));

    overload1 = 0;
    f();
    BOOST_CHECK_EQUAL(overload1, 1);

    overload2 = 0;
    f(2);
    BOOST_CHECK_EQUAL(overload2, 2);

    overload3 = 0;
    f(a);
    BOOST_CHECK_EQUAL(overload3, 3);
    
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>()>::type, void>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>(int)>::type, void>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>(any<test_concept, _a>)>::type, void>));
}

BOOST_AUTO_TEST_CASE(test_overload_return)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<int()>,
        callable<int(int)>,
        callable<int(_a)>
    > test_concept;
    tuple<test_concept, _self, _a> t(overloaded_function(), static_cast<short>(3));
    any<test_concept> f(get<0>(t));
    any<test_concept, _a> a(get<1>(t));

    overload1 = 0;
    BOOST_CHECK_EQUAL(f(), 1);
    BOOST_CHECK_EQUAL(overload1, 1);

    overload2 = 0;
    BOOST_CHECK_EQUAL(f(2), 2);
    BOOST_CHECK_EQUAL(overload2, 2);

    overload3 = 0;
    BOOST_CHECK_EQUAL(f(a), 3);
    BOOST_CHECK_EQUAL(overload3, 3);
    
    //BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>()>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>(int)>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>(any<test_concept, _a>)>::type, int>));
}


BOOST_AUTO_TEST_CASE(test_overload_const)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void(), const _self>,
        callable<void(int), const _self>,
        callable<void(_a), const _self>
    > test_concept;
    tuple<test_concept, _self, _a> t(overloaded_function(), static_cast<short>(3));
    any<test_concept> f(get<0>(t));
    any<test_concept, _a> a(get<1>(t));

    overload1 = 0;
    f();
    BOOST_CHECK_EQUAL(overload1, 1);

    overload2 = 0;
    f(2);
    BOOST_CHECK_EQUAL(overload2, 2);

    overload3 = 0;
    f(a);
    BOOST_CHECK_EQUAL(overload3, 3);
    
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>()>::type, void>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>(int)>::type, void>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>(any<test_concept, _a>)>::type, void>));
}

BOOST_AUTO_TEST_CASE(test_overload_return_const)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<int(), const _self>,
        callable<int(int), const _self>,
        callable<int(_a), const _self>
    > test_concept;
    tuple<test_concept, _self, _a> t(overloaded_function(), static_cast<short>(3));
    any<test_concept> f(get<0>(t));
    any<test_concept, _a> a(get<1>(t));

    overload1 = 0;
    BOOST_CHECK_EQUAL(f(), 1);
    BOOST_CHECK_EQUAL(overload1, 1);

    overload2 = 0;
    BOOST_CHECK_EQUAL(f(2), 2);
    BOOST_CHECK_EQUAL(overload2, 2);

    overload3 = 0;
    BOOST_CHECK_EQUAL(f(a), 3);
    BOOST_CHECK_EQUAL(overload3, 3);
    
    //BOOST_MPL_ASSERT((boost::is_same<boost::result_of<any<test_concept>()>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>(int)>::type, int>));
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<const any<test_concept>(any<test_concept, _a>)>::type, int>));
}

struct model_ret_ref
{
    model_ret_ref& operator()() { return *this; }
};

BOOST_AUTO_TEST_CASE(test_ref_any_result)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<_self&()>
    > test_concept;
    
    any<test_concept> x1 = model_ret_ref();
    any<test_concept, _self&> x2(x1());
    BOOST_CHECK_EQUAL(any_cast<model_ret_ref*>(&x1), any_cast<model_ret_ref*>(&x2));
}

int f_ret_ref_val;
int& f_ret_ref() { return f_ret_ref_val; }

BOOST_AUTO_TEST_CASE(test_ref_int_result)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<int&()>
    > test_concept;
    
    any<test_concept> x1 = f_ret_ref;
    int& result = x1();
    BOOST_CHECK_EQUAL(&result, &f_ret_ref_val);
}

struct model_ret_cref
{
    const model_ret_cref& operator()() { return *this; }
};

BOOST_AUTO_TEST_CASE(test_cref_any_result)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<const _self&()>
    > test_concept;
    
    any<test_concept> x1 = model_ret_ref();
    any<test_concept, const _self&> x2(x1());
    BOOST_CHECK_EQUAL(any_cast<const model_ret_cref*>(&x1), any_cast<const model_ret_cref*>(&x2));
}

int f_ret_cref_val;
const int& f_ret_cref() { return f_ret_cref_val; }

BOOST_AUTO_TEST_CASE(test_cref_int_result)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<const int&()>
    > test_concept;
    
    any<test_concept> x1 = f_ret_cref;
    const int& result = x1();
    BOOST_CHECK_EQUAL(&result, &f_ret_cref_val);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

int f_rv_value = 0;
void f_rv(int&& i) { f_rv_value += i; }

BOOST_AUTO_TEST_CASE(test_rvalue_int)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<void(int&&)>
    > test_concept;
    any<test_concept> f(&f_rv);
    
    f_rv_value = 1;
    f(2);
    BOOST_CHECK_EQUAL(f_rv_value, 3);
}

BOOST_AUTO_TEST_CASE(test_rvalue_any)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void(_a&&)>
    > test_concept;
    
    tuple<test_concept, _self, _a> t1(&f_rv, 3);
    any<test_concept> x1(get<0>(t1));
    f_rv_value = 1;
    x1(std::move(get<1>(t1)));
    BOOST_CHECK_EQUAL(f_rv_value, 4);
}

BOOST_AUTO_TEST_CASE(test_const_rvalue_int)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<void(int&&), const _self>
    > test_concept;
    const any<test_concept> f(&f_rv);
    
    f_rv_value = 1;
    f(2);
    BOOST_CHECK_EQUAL(f_rv_value, 3);
}

BOOST_AUTO_TEST_CASE(test_const_rvalue_any)
{
    typedef ::boost::mpl::vector<
        common<>,
        common<_a>,
        callable<void(_a&&), const _self>
    > test_concept;
    
    tuple<test_concept, _self, _a> t1(&f_rv, 3);
    const any<test_concept> x1(get<0>(t1));
    f_rv_value = 1;
    x1(std::move(get<1>(t1)));
    BOOST_CHECK_EQUAL(f_rv_value, 4);
}

struct model_ret_rref
{
    model_ret_rref&& operator()() { return std::move(*this); }
};

BOOST_AUTO_TEST_CASE(test_rvalue_any_result)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<_self&&()>
    > test_concept;
    
    any<test_concept> x1 = model_ret_rref();
    any<test_concept, _self&&> x2(x1());
    BOOST_CHECK_EQUAL(any_cast<model_ret_rref*>(&x1), any_cast<model_ret_rref*>(&x2));
}

int f_ret_rv_val;
int&& f_ret_rv() { return std::move(f_ret_rv_val); }

BOOST_AUTO_TEST_CASE(test_rvalue_int_result)
{
    typedef ::boost::mpl::vector<
        common<>,
        callable<int&&()>
    > test_concept;
    
    any<test_concept> x1 = f_ret_rv;
    int&& result = x1();
    BOOST_CHECK_EQUAL(&result, &f_ret_rv_val);
}

#endif
