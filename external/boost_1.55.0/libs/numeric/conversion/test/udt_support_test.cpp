// (C) Copyright 2003, Fernando Luis Cacciola Carballal.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#include<iostream>
#include<iomanip>
#include<string>
#include<typeinfo>
#include<vector>
#include<algorithm>

#include "boost/numeric/conversion/converter.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "test_helpers.cpp"
#include "test_helpers2.cpp"
#include "test_helpers3.cpp"

using namespace std ;
using namespace boost ;
using namespace numeric ;
using namespace MyUDT ;

//-------------------------------------------------------------------------
// These are the typical steps that are required to install support for
// conversions from/to UDT which need special treatment.
//-------------------------------------------------------------------------



//
// (1) Instantiate specific convesions traits.
//     This step is only for convenience.
//     These traits instances are required in order to define the specializations
//     that follow (and which *are required* to make the library work with MyInt and MyFloat)
//
namespace MyUDT {

typedef conversion_traits<double , MyFloat> MyFloat_to_double_Traits;
typedef conversion_traits<int    , MyFloat> MyFloat_to_int_Traits;
typedef conversion_traits<MyInt  , MyFloat> MyFloat_to_MyInt_Traits;
typedef conversion_traits<int    , MyInt  > MyInt_to_int_Traits;
typedef conversion_traits<MyFloat, MyInt  > MyInt_to_MyFloat_Traits;
typedef conversion_traits<MyInt  , double > double_to_MyInt_Traits;

} // namespace MyUDT


//
// (2) Define suitable raw converters.
//
//   Our sample UDTs don't support implicit conversions.
//   Therefore, the default raw_converter<> doesn't work,
//   and we need to define our own.
//
//   There are two ways of doing this:
//
//     (a) One is to simply specialize boost::numeric::raw_converter<> directly.
//         This way, the default converter will work out of the box, which means, for instance,
//         that numeric_cast<> can be used with these UDTs.
//
//     (b) Define a user class with the appropriate interface and supply it explicitely
//         as a policy to a converter instance.
//
//   This test uses chice (a).
//
namespace boost {

namespace numeric {

template<>
struct raw_converter<MyUDT::MyFloat_to_double_Traits>
{
  static double low_level_convert ( MyUDT::MyFloat const&  s )
    { return s.to_builtin() ; }
} ;

template<>
struct raw_converter<MyUDT::MyFloat_to_int_Traits>
{
  static int low_level_convert ( MyUDT::MyFloat const& s )
    { return static_cast<int>( s.to_builtin() ) ; }
} ;

template<>
struct raw_converter<MyUDT::MyFloat_to_MyInt_Traits>
{
  static MyUDT::MyInt low_level_convert ( MyUDT::MyFloat const& s )
    { return MyUDT::MyInt( static_cast<int>(s.to_builtin()) ) ; }
} ;

template<>
struct raw_converter<MyUDT::MyInt_to_int_Traits>
{
  static int low_level_convert ( MyUDT::MyInt const& s ) { return s.to_builtin() ; }
} ;

template<>
struct raw_converter<MyUDT::MyInt_to_MyFloat_Traits>
{
  static MyUDT::MyFloat low_level_convert ( MyUDT::MyInt const& s )
    {
      return MyUDT::MyFloat( static_cast<double>(s.to_builtin()) ) ;
    }
} ;

template<>
struct raw_converter<MyUDT::double_to_MyInt_Traits>
{
  static MyUDT::MyInt low_level_convert ( double s )
    { return MyUDT::MyInt( static_cast<int>(s) ) ; }
} ;

} // namespace numeric

} // namespace boost



//
// (3) Define suitable range checkers
//
// By default, if a UDT is involved in a conversion, internal range checking is disabled.
// This is so because a UDT type can have any sort of range, even unbounded, thus
// the library doesn't attempt to automatically figure out the appropriate range checking logic.
// (as it does when builtin types are involved)
// However, this situation is a bit unsufficient in practice, specially from doing narrowing (subranged)
// conversions from UDTs.
// The library provides a rudimentary hook to help this out: The user can plug in his own
// range checker to the converter instance.
//
// This test shows how to define and use a custom range checker.
//

namespace MyUDT {

//
// The following are metaprogramming tools to allow us the implement the
// MyCustomRangeChecker generically, for either builtin or UDT types.
//

// get_builtin_type<N>::type extracts the built-in type of our UDT's
//
template<class N> struct get_builtin_type { typedef N type ; } ;
template<> struct get_builtin_type<MyInt>   { typedef int type ; } ;
template<> struct get_builtin_type<MyFloat> { typedef double type ; } ;

// U extract_builtin ( T s ) returns 's' converted to the corresponding built-in type U.
//   
template<class N>
struct extract_builtin
{
  static N apply ( N n ) { return n ; }
} ;
template<>
struct extract_builtin<MyInt>
{
  static int apply ( MyInt const& n ) { return n.to_builtin() ; }
} ;
template<>
struct extract_builtin<MyFloat>
{
  static double apply ( MyFloat const& n ) { return n.to_builtin() ; }
} ;

template<class Traits>
struct MyCustomRangeChecker
{
  typedef typename Traits::argument_type argument_type ;

  // This custom range checker uses the fact that our 'fake' UDT are merely wrappers
  // around builtin types; so it just forward the logic to the correspoding range
  // checkers for the wrapped builtin types.
  //
  typedef typename Traits::source_type S ;
  typedef typename Traits::target_type T ;

  // NOTE: S and/or T can be either UDT or builtin types.

  typedef typename get_builtin_type<S>::type builtinS ;
  typedef typename get_builtin_type<T>::type builtinT ;

  // NOTE: The internal range checker used by default is *built* when you instantiate
  // a converter<> with a given Traits according to the properties of the involved types.
  // Currently, there is no way to instantiate this range checker as a separate class.
  // However, you can see it as part of the interface of the converter
  // (since the converter inherits from it)
  // Therefore, here we instantiate a converter corresponding to the builtin types to access
  // their associated builtin range checker.
  //
  typedef boost::numeric::converter<builtinT,builtinS> InternalConverter ;

  static range_check_result out_of_range ( argument_type s )
    {
      return InternalConverter::out_of_range( extract_builtin<S>::apply(s) );
    }

  static void validate_range ( argument_type s )
    {
      return InternalConverter::validate_range( extract_builtin<S>::apply(s) );
    }
} ;

} // namespace MyUDT








//
// Test here
//

void test_udt_conversions_with_defaults()
{
  cout << "Testing UDT conversion with default policies\n" ;

  // MyInt <--> int

    int mibv = rand();
    MyInt miv(mibv);
    TEST_SUCCEEDING_CONVERSION_DEF(MyInt,int,miv,mibv);
    TEST_SUCCEEDING_CONVERSION_DEF(int,MyInt,mibv,miv);

  // MyFloat <--> double

    double mfbv = static_cast<double>(rand()) / 3.0 ;
    MyFloat mfv (mfbv);
    TEST_SUCCEEDING_CONVERSION_DEF(MyFloat,double,mfv,mfbv);
    TEST_SUCCEEDING_CONVERSION_DEF(double,MyFloat,mfbv,mfv);

  // MyInt <--> MyFloat

    MyInt   miv2  ( static_cast<int>(mfbv) );
    MyFloat miv2F ( static_cast<int>(mfbv) );
    MyFloat mfv2  ( static_cast<double>(mibv) );
    MyInt   mfv2I ( static_cast<double>(mibv) );
    TEST_SUCCEEDING_CONVERSION_DEF(MyFloat,MyInt,miv2F,miv2);
    TEST_SUCCEEDING_CONVERSION_DEF(MyInt,MyFloat,mfv2I,mfv2);
}

template<class T, class S>
struct GenerateCustomConverter
{
  typedef conversion_traits<T,S> Traits;

  typedef def_overflow_handler         OverflowHandler ;
  typedef Trunc<S>                     Float2IntRounder ;
  typedef raw_converter<Traits>        RawConverter ;
  typedef MyCustomRangeChecker<Traits> RangeChecker ;

  typedef converter<T,S,Traits,OverflowHandler,Float2IntRounder,RawConverter,RangeChecker> type ;
} ;

void test_udt_conversions_with_custom_range_checking()
{
  cout << "Testing UDT conversions with custom range checker\n" ;

  int mibv = rand();
  MyFloat mfv ( static_cast<double>(mibv) );

  typedef GenerateCustomConverter<MyFloat,int>::type int_to_MyFloat_Conv ;

  TEST_SUCCEEDING_CONVERSION( int_to_MyFloat_Conv, MyFloat, int, mfv, mibv );

  int mibv2 = rand();
  MyInt miv (mibv2);
  MyFloat mfv2 ( static_cast<double>(mibv2) );

  typedef GenerateCustomConverter<MyFloat,MyInt>::type MyInt_to_MyFloat_Conv ;

  TEST_SUCCEEDING_CONVERSION( MyInt_to_MyFloat_Conv, MyFloat, MyInt, mfv2, miv );

  double mfbv = bounds<double>::highest();
  typedef GenerateCustomConverter<MyInt,double>::type double_to_MyInt_Conv ;

  TEST_POS_OVERFLOW_CONVERSION( double_to_MyInt_Conv, MyInt, double, mfbv );

  MyFloat mfv3 ( bounds<double>::lowest() ) ;
  typedef GenerateCustomConverter<int,MyFloat>::type MyFloat_to_int_Conv ;

  TEST_NEG_OVERFLOW_CONVERSION( MyFloat_to_int_Conv, int, MyFloat, mfv3 );
}


int test_main( int, char* [] )
{
  cout << setprecision( numeric_limits<long double>::digits10 ) ;

  test_udt_conversions_with_defaults();
  test_udt_conversions_with_custom_range_checking();

  return 0;
}






