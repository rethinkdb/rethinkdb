
#ifdef _MSC_VER
#pragma pack(2)
#endif

//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/alignment_of.hpp>
#endif

//
// Need to defined some member functions for empty_UDT,
// we don't want to put these in the test.hpp as that
// causes overly-clever compilers to figure out that they can't throw
// which in turn breaks other tests.
//
empty_UDT::empty_UDT(){}
empty_UDT::~empty_UDT(){}
empty_UDT::empty_UDT(const empty_UDT&){}
empty_UDT& empty_UDT::operator=(const empty_UDT&){ return *this; }
bool empty_UDT::operator==(const empty_UDT&)const{ return true; }


//
// VC++ emits an awful lot of warnings unless we define these:
#ifdef BOOST_MSVC
#  pragma warning(disable:4244 4121)
//
// What follows here is the test case for issue 1946.
//
#include <boost/function.hpp>
// This kind of packing is set within MSVC 9.0 headers.
// E.g. std::ostream has it.
#pragma pack(push,8)

// The issue is gone if Root has no data members
struct Root { int a; };
// The issue is gone if Root is inherited non-virtually
struct A : virtual public Root {};

#pragma pack(pop)
//
// This class has 8-byte alignment but is 44 bytes in size, which means 
// that elements in an array of this type will not actually be 8 byte
// aligned.  This appears to be an MSVC bug, and throws off our 
// alignment calculations: causing us to report a non-sensical 12-byte
// alignment for this type.  This is fixed by using the native __alignof
// operator.
//
class issue1946 :
    public A
{
public:
    // The issue is gone if the type is not a boost::function. The signature doesn't matter.
    typedef boost::function0< void > function_type;
    function_type m_function;
};

#endif


template <class T>
struct align_calc
{
   char padding;
   T instance;
   static std::ptrdiff_t get()
   {
      static align_calc<T> a;
      return reinterpret_cast<const char*>(&(a.instance)) - reinterpret_cast<const char*>(&(a.padding));
   }
};

#define ALIGNOF(x) align_calc< x>::get()

TT_TEST_BEGIN(alignment_of)

#ifndef TEST_STD
// This test is not required to work for non-boost implementations:
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<void>::value, 0);
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<char>::value, ALIGNOF(char));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<short>::value, ALIGNOF(short));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<int>::value, ALIGNOF(int));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<long>::value, ALIGNOF(long));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<float>::value, ALIGNOF(float));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<double>::value, ALIGNOF(double));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<long double>::value, ALIGNOF(long double));
#ifdef BOOST_HAS_LONG_LONG
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of< ::boost::long_long_type>::value, ALIGNOF(::boost::long_long_type));
#endif
#ifdef BOOST_HAS_MS_INT64
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<__int64>::value, ALIGNOF(__int64));
#endif
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<int[4]>::value, ALIGNOF(int[4]));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<int(*)(int)>::value, ALIGNOF(int(*)(int)));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<int*>::value, ALIGNOF(int*));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<VB>::value, ALIGNOF(VB));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<VD>::value, ALIGNOF(VD));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<enum_UDT>::value, ALIGNOF(enum_UDT));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<mf2>::value, ALIGNOF(mf2));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<POD_UDT>::value, ALIGNOF(POD_UDT));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<empty_UDT>::value, ALIGNOF(empty_UDT));
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<union_UDT>::value, ALIGNOF(union_UDT));

#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1400)
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::alignment_of<issue1946>::value, ALIGNOF(issue1946));
#endif

TT_TEST_END








