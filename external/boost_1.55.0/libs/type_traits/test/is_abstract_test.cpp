
//  (C) Copyright Rani Sharoni,Robert Ramey, Pavel Vozenilek and Christoph Ludwig 2004. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_abstract.hpp>
#endif

#ifdef BOOST_MSVC
#pragma warning(disable: 4505)
#endif


struct TestA {};
struct TestB { virtual void foo(void) = 0; };
struct TestC { private: virtual void foo(void) = 0; };
struct TestD : public TestA {};
struct TestE : public TestB {};
struct TestF : public TestC {};
struct TestG : public TestB { virtual void foo(void) {} };
struct TestH : public TestC { private: virtual void foo(void) {} };
struct TestI : public TestB, public TestC {};
struct TestJ : public TestI { virtual void foo(void) {} };
struct TestK : public TestB { virtual void foo(void); virtual void foo2(void) = 0; };
struct TestL : public TestK { virtual void foo2(void) {} };
struct TestM : public virtual TestB {};
struct TestN : public virtual TestC {};
struct TestO : public TestM, public TestN {};
struct TestP : public TestO { virtual void foo(void) {} };
struct TestQ : public TestB { virtual void foo(void) = 0; };
struct TestR : public TestC { private: virtual void foo(void) = 0; };
struct TestS { virtual void foo(void) {} };
struct TestT { virtual ~TestT(void) {} virtual void foo(void) {} };
struct TestU : public TestT { virtual void foo(void) = 0; };
struct TestV : public TestT { virtual void foo(void) {} };
struct TestW { virtual void foo1(void) = 0; virtual void foo2(void) = 0; };
struct TestX : public TestW { virtual void foo1(void) {}  virtual void foo2(void) {} };
struct TestY { virtual ~TestY(void) = 0; };
struct TestZ { virtual ~TestZ(void) = 0; }; TestZ::~TestZ(void) {}
struct TestAA : public TestZ { virtual ~TestAA(void) = 0; }; TestAA::~TestAA(void) {}
struct TestAB : public TestAA { virtual ~TestAB(void) {} }; 
struct TestAC { virtual void foo(void) = 0; }; void TestAC::foo(void) {}
struct TestAD : public TestAC {};
struct TestAE : public TestAD { virtual void foo() {} };
struct TestAF : public TestAD { virtual void foo(); }; void TestAF::foo(void) {}
struct TestAG : public virtual TestA {};

// template test types:
template <class T> struct TTestA {};
template <class T> struct TTestB { virtual void foo(void) = 0; };
template <class T> struct TTestC { private: virtual void foo(void) = 0; };
template <class T> struct TTestD : public TTestA<T> {};
template <class T> struct TTestE : public TTestB<T> {};
template <class T> struct TTestF : public TTestC<T> {};
template <class T> struct TTestG : public TTestB<T> { virtual void foo(void) {} };
template <class T> struct TTestH : public TTestC<T> { private: virtual void foo(void) {} };
template <class T> struct TTestI : public TTestB<T>, public TTestC<T> {};
template <class T> struct TTestJ : public TTestI<T> { virtual void foo(void) {} };
template <class T> struct TTestK : public TTestB<T> { virtual void foo(void); virtual void foo2(void) = 0; };
template <class T> struct TTestL : public TTestK<T> { virtual void foo2(void) {} };
template <class T> struct TTestM : public virtual TTestB<T> {};
template <class T> struct TTestN : public virtual TTestC<T> {};
template <class T> struct TTestO : public TTestM<T>, public TTestN<T> {};
template <class T> struct TTestP : public TTestO<T> { virtual void foo(void) {} };
template <class T> struct TTestQ : public TTestB<T> { virtual void foo(void) = 0; };
template <class T> struct TTestR : public TTestC<T> { private: virtual void foo(void) = 0; };
template <class T> struct TTestS { virtual void foo(void) {} };
template <class T> struct TTestT { virtual ~TTestT(void) {} virtual void foo(void) {} };
template <class T> struct TTestU : public TTestT<T> { virtual void foo(void) = 0; };
template <class T> struct TTestV : public TTestT<T> { virtual void foo(void) {} };
template <class T> struct TTestW { virtual void foo1(void) = 0; virtual void foo2(void) = 0; };
template <class T> struct TTestX : public TTestW<T> { virtual void foo1(void) {}  virtual void foo2(void) {} };
template <class T> struct TTestY { virtual ~TTestY(void) = 0; };
template <class T> struct TTestZ { virtual ~TTestZ(void) = 0; }; template <class T> TTestZ<T>::~TTestZ(void) {}
template <class T> struct TTestAA : public TTestZ<T> { virtual ~TTestAA(void) = 0; }; template <class T> TTestAA<T>::~TTestAA(void) {}
template <class T> struct TTestAB : public TTestAA<T> { virtual ~TTestAB(void) {} }; 
template <class T> struct TTestAC { virtual void foo(void) = 0; }; template <class T> void TTestAC<T>::foo(void) {}
template <class T> struct TTestAD : public TTestAC<T> {};
template <class T> struct TTestAE : public TTestAD<T> { virtual void foo() {} };
template <class T> struct TTestAF : public TTestAD<T> { virtual void foo(); }; template <class T> void TTestAF<T>::foo(void) {}
template <class T> struct TTestAG : public virtual TTestA<T> {};


TT_TEST_BEGIN(is_abstract)

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestA>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestB>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestD>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestE>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestF>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestG>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestH>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestI>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestJ>::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestK>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestL>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestM>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestN>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestO>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestP>::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestQ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestR>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestS>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestT>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestU>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestV>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestW>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestX>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestY>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestZ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAA>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAB>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAD>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAE>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAF>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAG>::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestA>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestB>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestD>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestE>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestF>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestG>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestH>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestI>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestJ>::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestK>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestL>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestM>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestN>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestO>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestP>::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestQ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestR>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestS>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestT>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestU>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestV>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestW>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestX>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestY>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestZ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestAA>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestAB>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestAC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestAD>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestAE>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestAF>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TestAG>::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestA>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestB>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestD>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestE>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestF>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestG>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestH>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestI>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestJ>::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestK>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestL>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestM>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestN>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestO>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestP>::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestQ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestR>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestS>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestT>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestU>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestV>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestW>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestX>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestY>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestZ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestAA>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestAB>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestAC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestAD>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestAE>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestAF>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TestAG>::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestA>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestB>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestD>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestE>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestF>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestG>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestH>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestI>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestJ>::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestK>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestL>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestM>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestN>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestO>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestP>::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestQ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestR>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestS>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestT>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestU>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestV>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestW>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestX>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestY>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestZ>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestAA>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestAB>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestAC>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestAD>::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestAE>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestAF>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TestAG>::value), false);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestA&&>::value), false);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestA&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestB&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestC&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestD&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestE&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestF&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestG&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestH&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestI&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestJ&>::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestK&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestL&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestM&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestN&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestO&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestP&>::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestQ&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestR&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestS&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestT&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestU&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestV&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestW&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestX&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestY&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestZ&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAA&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAB&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAC&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAD&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAE&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAF&>::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TestAG&>::value), false);
#if !(defined(BOOST_MSVC) && (BOOST_MSVC < 1310))
// MSVC prior to VC7.1 always runs out of swap space trying to
// compile these, so leave them out for now (the test fails anyway).
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestA<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestB<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestD<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestE<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestF<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestG<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestH<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestI<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestJ<int> >::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestK<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestL<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestM<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestN<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestO<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestP<int> >::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestQ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestR<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestS<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestT<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestU<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestV<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestW<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestX<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestY<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestZ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAA<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAB<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAD<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAE<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAF<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAG<int> >::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestA<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestB<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestD<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestE<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestF<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestG<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestH<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestI<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestJ<int> >::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestK<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestL<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestM<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestN<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestO<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestP<int> >::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestQ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestR<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestS<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestT<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestU<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestV<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestW<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestX<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestY<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestZ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestAA<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestAB<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestAC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestAD<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestAE<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestAF<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<const TTestAG<int> >::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestA<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestB<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestD<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestE<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestF<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestG<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestH<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestI<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestJ<int> >::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestK<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestL<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestM<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestN<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestO<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestP<int> >::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestQ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestR<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestS<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestT<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestU<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestV<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestW<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestX<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestY<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestZ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestAA<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestAB<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestAC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestAD<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestAE<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestAF<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile TTestAG<int> >::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestA<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestB<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestD<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestE<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestF<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestG<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestH<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestI<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestJ<int> >::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestK<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestL<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestM<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestN<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestO<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestP<int> >::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestQ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestR<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestS<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestT<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestU<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestV<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestW<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestX<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestY<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestZ<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestAA<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestAB<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestAC<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestAD<int> >::value), true);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestAE<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestAF<int> >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<volatile const TTestAG<int> >::value), false);

BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestA<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestB<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestC<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestD<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestE<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestF<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestG<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestH<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestI<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestJ<int>& >::value), false); // only one method implemented!
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestK<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestL<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestM<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestN<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestO<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestP<int>& >::value), false); // ???
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestQ<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestR<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestS<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestT<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestU<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestV<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestW<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestX<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestY<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestZ<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAA<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAB<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAC<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAD<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAE<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAF<int>& >::value), false);
BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_abstract<TTestAG<int>& >::value), false);
#endif
TT_TEST_END







