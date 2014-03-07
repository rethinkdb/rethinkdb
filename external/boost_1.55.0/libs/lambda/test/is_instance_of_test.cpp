// is_instance_of_test.cpp -- The Boost Lambda Library ------------------
//
// Copyright (C) 2000-2003 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (C) 2000-2003 Gary Powell (powellg@amazon.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// -----------------------------------------------------------------------


#include <boost/test/minimal.hpp>    // see "Header Implementation Option"
 

#include "boost/lambda/detail/is_instance_of.hpp"

#include <iostream>

template <class T1> struct A1 {};
template <class T1, class T2> struct A2 {};
template <class T1, class T2, class T3> struct A3 {};
template <class T1, class T2, class T3, class T4> struct A4 {};
 
class B1 : public A1<int> {};
class B2 : public A2<int,int> {};
class B3 : public A3<int,int,int> {};
class B4 : public A4<int,int,int,int> {};

// classes that are convertible to classes that derive from A instances 
// This is not enough to make the test succeed

class C1 { public: operator A1<int>() { return A1<int>(); } };
class C2 { public: operator B2() { return B2(); } };
class C3 { public: operator B3() { return B3(); } };
class C4 { public: operator B4() { return B4(); } };
 
// test that the result is really a constant
// (in an alternative implementation, gcc 3.0.2. claimed that it was 
// a non-constant)
template <bool b> class X {};
// this should compile 
X<boost::lambda::is_instance_of_2<int, A2>::value> x;


int test_main(int, char *[]) {

using boost::lambda::is_instance_of_1;
using boost::lambda::is_instance_of_2;
using boost::lambda::is_instance_of_3;
using boost::lambda::is_instance_of_4;


BOOST_CHECK((is_instance_of_1<B1, A1>::value == true));
BOOST_CHECK((is_instance_of_1<A1<float>, A1>::value == true));
BOOST_CHECK((is_instance_of_1<int, A1>::value == false));
BOOST_CHECK((is_instance_of_1<C1, A1>::value == false));

BOOST_CHECK((is_instance_of_2<B2, A2>::value == true));
BOOST_CHECK((is_instance_of_2<A2<int, float>, A2>::value == true));
BOOST_CHECK((is_instance_of_2<int, A2>::value == false));
BOOST_CHECK((is_instance_of_2<C2, A2>::value == false));

BOOST_CHECK((is_instance_of_3<B3, A3>::value == true));
BOOST_CHECK((is_instance_of_3<A3<int, float, char>, A3>::value == true));
BOOST_CHECK((is_instance_of_3<int, A3>::value == false));
BOOST_CHECK((is_instance_of_3<C3, A3>::value == false));

BOOST_CHECK((is_instance_of_4<B4, A4>::value == true));
BOOST_CHECK((is_instance_of_4<A4<int, float, char, double>, A4>::value == true));
BOOST_CHECK((is_instance_of_4<int, A4>::value == false));
BOOST_CHECK((is_instance_of_4<C4, A4>::value == false));

return 0;

}

