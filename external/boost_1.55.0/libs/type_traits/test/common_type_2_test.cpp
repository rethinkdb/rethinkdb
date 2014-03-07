//  common_type_test.cpp  ----------------------------------------------------//

//  Copyright 2010 Beman Dawes

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#define BOOST_COMMON_TYPE_DONT_USE_TYPEOF 1

#include "test.hpp"
#include "check_type.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/common_type.hpp>
#endif
#include <iostream>

#ifdef BOOST_INTEL
#pragma warning(disable: 304 383)
#endif

struct C1 {};
    
struct C2 {};

    
struct C3 : C2 {};
struct C1C2 {
    C1C2() {}
    C1C2(C1 const&) {}
    C1C2(C2 const&) {}
    C1C2& operator=(C1C2 const&) {
        return *this;
    }
};

template <typename C, typename A>
void proc2(typename boost::common_type<A, C>::type const& ) {}

template <typename C, typename A, typename B>
void proc3(typename boost::common_type<C, A, B>::type const& ) {}

template <typename C, typename A>
void assignation_2() {
typedef typename boost::common_type<A, C>::type AC;
    A a;
    C c;
    AC ac;
    ac=a;
    ac=c;

    proc2<C, A>(a);
    proc2<C, A>(c);
    
}

template <typename C, typename A, typename B>
void assignation_3() {
typedef typename boost::common_type<C, A, B>::type ABC;
    A a;
    B b;
    C c;
    ABC abc;
    
    abc=a;
    abc=b;
    abc=c;
    
    proc3<C, A, B>(a);
    proc3<C, A, B>(b);
    proc3<C, A, B>(c);
}

C1C2 c1c2;
C1 c1;

int f(C1C2 ) { return 1;}
int f(C1 ) { return 2;}
template <typename OSTREAM>
OSTREAM& operator<<(OSTREAM& os, C1 const&) {return os;}

C1C2& declval_C1C2() {return c1c2;}
C1& declval_C1(){return c1;}
bool declval_bool(){return true;}


TT_TEST_BEGIN(common_type)
{
#ifndef __SUNPRO_CC
    assignation_2<C1C2, C1>();
    typedef tt::common_type<C1C2&, C1&>::type T1;
    typedef tt::common_type<C3*, C2*>::type T2;
    typedef tt::common_type<int*, int const*>::type T3;
#if defined(BOOST_NO_CXX11_DECLTYPE) && !defined(BOOST_COMMON_TYPE_DONT_USE_TYPEOF)
    // fails if BOOST_COMMON_TYPE_DONT_USE_TYPEOF:
    typedef tt::common_type<int volatile*, int const*>::type T4;
#endif
    typedef tt::common_type<int*, int volatile*>::type T5;

    assignation_2<C1, C1C2>();
    assignation_2<C1C2, C2>();
    assignation_2<C2, C1C2>();
    assignation_3<C1, C1C2, C2>();
    assignation_3<C1C2, C1, C2>();
    assignation_3<C2, C1C2, C1>();
    assignation_3<C1C2, C2, C1>();
    //assignation_3<C1, C2, C1C2>(); // fails because the common type is the third
#endif

    typedef tt::common_type<C1C2, C1>::type t1;
    BOOST_CHECK_TYPE(t1, C1C2);

    BOOST_CHECK_TYPE(tt::common_type<int>::type, int);
    BOOST_CHECK_TYPE(tt::common_type<char>::type, char);
    
    BOOST_CHECK_TYPE3(tt::common_type<char, char>::type, char);
    BOOST_CHECK_TYPE3(tt::common_type<char, unsigned char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<char, short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<char, unsigned short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<char, int>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<char, unsigned int>::type, unsigned int);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<char, boost::long_long_type>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<char, boost::ulong_long_type>::type, boost::ulong_long_type);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<char, double>::type, double);

    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, unsigned char>::type, unsigned char);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, unsigned short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, int>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, unsigned int>::type, unsigned int);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, boost::long_long_type>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, boost::ulong_long_type>::type, boost::ulong_long_type);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<unsigned char, double>::type, double);
    
    BOOST_CHECK_TYPE3(tt::common_type<short, char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<short, unsigned char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<short, short>::type, short);
    BOOST_CHECK_TYPE3(tt::common_type<short, unsigned short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<short, int>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<short, unsigned int>::type, unsigned int);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<short, boost::long_long_type>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<short, boost::ulong_long_type>::type, boost::ulong_long_type);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<short, double>::type, double);

    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, unsigned char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, unsigned short>::type, unsigned short);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, int>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, unsigned int>::type, unsigned int);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, boost::long_long_type>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, boost::ulong_long_type>::type, boost::ulong_long_type);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<unsigned short, double>::type, double);

    BOOST_CHECK_TYPE3(tt::common_type<int, char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<int, unsigned char>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<int, short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<int, unsigned short>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<int, int>::type, int);
    BOOST_CHECK_TYPE3(tt::common_type<int, unsigned int>::type, unsigned int);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<int, boost::long_long_type>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<int, boost::ulong_long_type>::type, boost::ulong_long_type);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<int, double>::type, double);

    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, char>::type, unsigned int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, unsigned char>::type, unsigned int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, short>::type, unsigned int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, unsigned short>::type, unsigned int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, int>::type, unsigned int);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, unsigned int>::type, unsigned int);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, boost::long_long_type>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, boost::ulong_long_type>::type, boost::ulong_long_type);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<unsigned int, double>::type, double);

#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, char>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, unsigned char>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, short>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, unsigned short>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, int>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, unsigned int>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, boost::long_long_type>::type, boost::long_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, boost::ulong_long_type>::type, boost::ulong_long_type);
    BOOST_CHECK_TYPE3(tt::common_type<boost::long_long_type, double>::type, double);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<double, char>::type, double);
    BOOST_CHECK_TYPE3(tt::common_type<double, unsigned char>::type, double);
    BOOST_CHECK_TYPE3(tt::common_type<double, short>::type, double);
    BOOST_CHECK_TYPE3(tt::common_type<double, unsigned short>::type, double);
    BOOST_CHECK_TYPE3(tt::common_type<double, int>::type, double);
    BOOST_CHECK_TYPE3(tt::common_type<double, unsigned int>::type, double);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE3(tt::common_type<double, boost::long_long_type>::type, double);
    BOOST_CHECK_TYPE3(tt::common_type<double, boost::ulong_long_type>::type, double);
#endif
    BOOST_CHECK_TYPE3(tt::common_type<double, double>::type, double);
    
    BOOST_CHECK_TYPE4(tt::common_type<double, char, int>::type, double);
#ifndef BOOST_NO_LONG_LONG
    BOOST_CHECK_TYPE4(tt::common_type<unsigned, char, boost::long_long_type>::type, boost::long_long_type);
#endif
}
TT_TEST_END
