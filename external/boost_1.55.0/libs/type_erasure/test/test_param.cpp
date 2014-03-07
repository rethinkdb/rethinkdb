// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_param.cpp 83248 2013-03-02 18:15:06Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/param.hpp>
#include <boost/type_erasure/builtin.hpp>

using namespace boost::type_erasure;

template<int N>
struct size { typedef char (&type)[N]; };

// lvalues
extern any<copy_constructible<>, _self> a1;
extern const any<copy_constructible<>, _self> a2;
extern any<copy_constructible<>, _self&> a3;
extern const any<copy_constructible<>, _self&> a4;
extern any<copy_constructible<>, const _self&> a5;
extern const any<copy_constructible<>, const _self&> a6;

// rvalues
any<copy_constructible<>, _self> a7();
const any<copy_constructible<>, _self> a8();
any<copy_constructible<>, _self&> a9();
const any<copy_constructible<>, _self&> a10();
any<copy_constructible<>, const _self&> a11();
const any<copy_constructible<>, const _self&> a12();

extern int i;

size<1>::type f1(param<copy_constructible<>, _self&>);
size<2>::type f1(...);

void test_ref() {
    BOOST_STATIC_ASSERT(sizeof(f1(a1)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f1(a2)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f1(a3)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f1(a4)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f1(a5)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f1(a6)) == 2);
    
    BOOST_STATIC_ASSERT(sizeof(f1(a7())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f1(a8())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f1(a9())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f1(a10())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f1(a11())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f1(a12())) == 2);

    BOOST_STATIC_ASSERT(sizeof(f1(i)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f1(1)) == 2);

    // Make sure that the constructors are actually instantiated
    param<copy_constructible<>, _self&> c1 = a1;
    // param<copy_constructible<>, _self&> c2 = a2;
    param<copy_constructible<>, _self&> c3 = a3;
    param<copy_constructible<>, _self&> c4 = a4;
    // param<copy_constructible<>, _self&> c5 = a5;
    // param<copy_constructible<>, _self&> c6 = a6;
    
    // param<copy_constructible<>, _self&> c7 = a7();
    // param<copy_constructible<>, _self&> c8 = a8();
    param<copy_constructible<>, _self&> c9 = a9();
    param<copy_constructible<>, _self&> c10 = a10();
    // param<copy_constructible<>, _self&> c11 = a11();
    // param<copy_constructible<>, _self&> c12 = a12();
}

size<1>::type f2(param<copy_constructible<>, const _self&>);
size<2>::type f2(...);

void test_cref() {
    BOOST_STATIC_ASSERT(sizeof(f2(a1)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a2)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a3)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a4)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a5)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a6)) == 1);
    
    BOOST_STATIC_ASSERT(sizeof(f2(a7())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a8())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a9())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a10())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a11())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f2(a12())) == 1);

    BOOST_STATIC_ASSERT(sizeof(f2(i)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f2(1)) == 2);

    // Make sure that the constructors are actually instantiated
    param<copy_constructible<>, const _self&> c1 = a1;
    param<copy_constructible<>, const _self&> c2 = a2;
    param<copy_constructible<>, const _self&> c3 = a3;
    param<copy_constructible<>, const _self&> c4 = a4;
    param<copy_constructible<>, const _self&> c5 = a5;
    param<copy_constructible<>, const _self&> c6 = a6;
    
    param<copy_constructible<>, const _self&> c7 = a7();
    param<copy_constructible<>, const _self&> c8 = a8();
    param<copy_constructible<>, const _self&> c9 = a9();
    param<copy_constructible<>, const _self&> c10 = a10();
    param<copy_constructible<>, const _self&> c11 = a11();
    param<copy_constructible<>, const _self&> c12 = a12();
}

size<1>::type f3(param<copy_constructible<>, _self>);
size<2>::type f3(...);

void test_val() {
    BOOST_STATIC_ASSERT(sizeof(f3(a1)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a2)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a3)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a4)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a5)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a6)) == 1);
    
    BOOST_STATIC_ASSERT(sizeof(f3(a7())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a8())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a9())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a10())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a11())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f3(a12())) == 1);

    BOOST_STATIC_ASSERT(sizeof(f3(i)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f3(1)) == 2);

    // Make sure that the constructors are actually instantiated
    param<copy_constructible<>, _self> c1 = a1;
    param<copy_constructible<>, _self> c2 = a2;
    param<copy_constructible<>, _self> c3 = a3;
    param<copy_constructible<>, _self> c4 = a4;
    param<copy_constructible<>, _self> c5 = a5;
    param<copy_constructible<>, _self> c6 = a6;
    
    param<copy_constructible<>, _self> c7 = a7();
    param<copy_constructible<>, _self> c8 = a8();
    param<copy_constructible<>, _self> c9 = a9();
    param<copy_constructible<>, _self> c10 = a10();
    param<copy_constructible<>, _self> c11 = a11();
    param<copy_constructible<>, _self> c12 = a12();
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

size<1>::type f4(param<copy_constructible<>, _self&&>);
size<2>::type f4(...);

void test_rref() {
    BOOST_STATIC_ASSERT(sizeof(f4(a1)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a2)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a3)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a4)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a5)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a6)) == 2);
    
    BOOST_STATIC_ASSERT(sizeof(f4(a7())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f4(a8())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a9())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a10())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a11())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(a12())) == 2);

    BOOST_STATIC_ASSERT(sizeof(f4(i)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f4(1)) == 2);

    // Make sure that the constructors are actually instantiated
    // param<copy_constructible<>, _self&&> c1 = a1;
    // param<copy_constructible<>, _self&&> c2 = a2;
    // param<copy_constructible<>, _self&&> c3 = a3;
    // param<copy_constructible<>, _self&&> c4 = a4;
    // param<copy_constructible<>, _self&&> c5 = a5;
    // param<copy_constructible<>, _self&&> c6 = a6;
    
    param<copy_constructible<>, _self&&> c7 = a7();
    // param<copy_constructible<>, _self&&> c8 = a8();
    // param<copy_constructible<>, _self&&> c9 = a9();
    // param<copy_constructible<>, _self&&> c10 = a10();
    // param<copy_constructible<>, _self&&> c11 = a11();
    // param<copy_constructible<>, _self&&> c12 = a12();
}

#endif

#ifndef BOOST_NO_FUNCTION_REFERENCE_QUALIFIERS

// Test conversion sequence rank

size<1>::type f5(param<copy_constructible<>, _self&>);
size<2>::type f5(param<copy_constructible<>, const _self&>);

void test_ref_cref() {
    BOOST_STATIC_ASSERT(sizeof(f5(a1)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f5(a2)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f5(a3)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f5(a4)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f5(a5)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f5(a6)) == 2);
    
    BOOST_STATIC_ASSERT(sizeof(f5(a7())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f5(a8())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f5(a9())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f5(a10())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f5(a11())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f5(a12())) == 2);
}

size<1>::type f6(param<copy_constructible<>, _self&>);
size<2>::type f6(param<copy_constructible<>, _self&&>);

void test_ref_rref() {
    BOOST_STATIC_ASSERT(sizeof(f6(a1)) == 1);
    // BOOST_STATIC_ASSERT(sizeof(f6(a2)) == 2);
    BOOST_STATIC_ASSERT(sizeof(f6(a3)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f6(a4)) == 1);
    // BOOST_STATIC_ASSERT(sizeof(f6(a5)) == 2);
    // BOOST_STATIC_ASSERT(sizeof(f6(a6)) == 2);
    
    BOOST_STATIC_ASSERT(sizeof(f6(a7())) == 2);
    // BOOST_STATIC_ASSERT(sizeof(f6(a8())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f6(a9())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f6(a10())) == 1);
    // BOOST_STATIC_ASSERT(sizeof(f6(a11())) == 2);
    // BOOST_STATIC_ASSERT(sizeof(f6(a12())) == 2);
}

size<1>::type f7(param<copy_constructible<>, const _self&>);
size<2>::type f7(param<copy_constructible<>, _self&&>);

void test_cref_rref() {
    BOOST_STATIC_ASSERT(sizeof(f7(a1)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a2)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a3)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a4)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a5)) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a6)) == 1);
    
    BOOST_STATIC_ASSERT(sizeof(f7(a7())) == 2);
    BOOST_STATIC_ASSERT(sizeof(f7(a8())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a9())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a10())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a11())) == 1);
    BOOST_STATIC_ASSERT(sizeof(f7(a12())) == 1);
}

#endif
