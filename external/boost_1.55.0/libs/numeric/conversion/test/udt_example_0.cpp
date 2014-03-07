// Copyright (C) 2005, Fernando Luis Cacciola Carballal.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#include "boost/config.hpp"
#include "boost/utility.hpp"
#include "boost/limits.hpp"
#include "boost/utility.hpp"

#include<iostream>
#include<iomanip>
#include<string>
#include<cmath>


#include "boost/test/included/test_exec_monitor.hpp"

#include "boost/numeric/conversion/cast.hpp"

using namespace std ;
using namespace boost;
using namespace numeric;

//
// This example illustrates how to add support for user defined types (UDTs)
// to the Boost Numeric Conversion Library.
// It is assumed that you are familiar with the following documentation:
//
//

//
// The minimum requirement is that boost::is_arithmetic<UDT> evaluates to false
// (Otherwise the converter code will try to examine the UDT as a built-in type)
//

//
// Let's start with the simpliest case of an UDT which supports standard conversions
//
struct Double
{
  Double( double v ) : mV(v) {}

  operator double() const { return mV ; }

  double mV ;
} ;

double dv = (numeric_limits<double>::max)() ;
double fv = (numeric_limits<float >::max)() ;
Double Dv(dv);
Double Fv(fv);

void simplest_case()
{
  //
  // conversion_traits<>::udt_builtin_mixture works out of the box as long as boost::is_arithmetic<UDT> yields false
  //
  BOOST_CHECK( (conversion_traits<double,Double>::udt_builtin_mixture::value == udt_to_builtin) ) ;
  BOOST_CHECK( (conversion_traits<Double,double>::udt_builtin_mixture::value == builtin_to_udt) ) ;
  BOOST_CHECK( (conversion_traits<Double,Double>::udt_builtin_mixture::value == udt_to_udt    ) ) ;

  // BY DEFINITION, a conversion from UDT to Builtin is subranged. No attempt is made to actually compare ranges.
  BOOST_CHECK( (conversion_traits<double,Double>::subranged::value) == true  ) ;
  BOOST_CHECK( (conversion_traits<Double,double>::subranged::value) == false ) ;



  //
  // Conversions to/from FLOATING types, if already supported by an UDT
  // are also supported out-of-the-box by converter<> in its default configuration.
  //
  BOOST_CHECK( numeric_cast<double>(Dv) == static_cast<double>(Dv) ) ;
  BOOST_CHECK( numeric_cast<Double>(dv) == static_cast<Double>(dv) ) ;

  BOOST_CHECK( numeric_cast<float> (Dv) == static_cast<float> (Dv) ) ;
  BOOST_CHECK( numeric_cast<Double>(fv) == static_cast<Double>(fv) ) ;


  //
  // Range checking is disabled by default if an UDT is either the source or target of the conversion.
  //
  BOOST_CHECK( (converter<float,double>::out_of_range(dv) == cPosOverflow) );
  BOOST_CHECK( (converter<float,Double>::out_of_range(Dv) == cInRange) );

}

//
// The conversion_traits<> class and therefore the converter<> class looks at
// numeric_limits<UDT>::is_integer/is_signed to generate the proper float_in and sign mixtures.
// In most implementations, is_integer/is_signed are both false for UDTs if there is no explicit specialization for it.
// Therefore, the converter<> will see any UDT for which numeric_limits<> is not specialized as Float AND unsigned.
// Signess is used in the converter<> for range checking, but range checking is disabled by default for UDTs, so,
// normally, signess is mostly irrelevant as far as the library is concerned, except for the numeric_traits<>::sign_mixture
// entry.
// is_integer, however, is relevant in that if the conversion is from a float type to an integer type, the conversion is
// "rounding" and the rounder policies will participate.
// ALL implemented rounder policies require proper definitions for floor(udt) and ceil(udt).
// These names will be searched for using ADL, so, if you need to convert TO integral types from a UDT,
// you need to supply those functions along with the UDT in right namespace (that is, any namespace that allows
// ADL to find them)

// If your UDT doesn't supply floor/ceil, conversions to integer types
// won't compile unless a custom Float2IntRounder is used.

Double floor ( Double v ) { return Double(std::floor(v.mV)) ; }
Double ceil  ( Double v ) { return Double(std::ceil (v.mV)) ; }

void rounding()
{
  BOOST_CHECK( numeric_cast<int>(Dv) == static_cast<int>(Dv) ) ;
}


//
// If your UDT can't or won't provide floor/ceil you can set-up and use your own
// Float2IntRounder policy (though doing this is not always required as shown so far)
//
struct DoubleToInt
{
  static Double nearbyint ( Double const& s ) { return Double(static_cast<int>(s)); }

  typedef mpl::integral_c< std::float_round_style, std::round_toward_zero> round_style ;
} ;

void custom_rounding()
{
  typedef converter<int
                   ,Double
                   ,conversion_traits<int,Double>
                   ,void // By default UDT disable range checking so this won't be used
                   ,DoubleToInt
                   >
                   DoubleToIntConverter ;

   BOOST_CHECK( DoubleToIntConverter::convert(Dv) == static_cast<int>(Dv) ) ;
}

//
// In the next Level of complexity, your UDTs might not support conversion operators
//
struct Float
{
  Float( float v ) : mV(v) {}

  float mV ;
} ;

struct Int
{
  Int( int v ) : mV(v) {}

  int mV ;
} ;

typedef conversion_traits<Int,Float> Float2IntTraits ;
typedef conversion_traits<Float,Int> Int2FloatTraits ;

namespace boost { namespace numeric
{
//
// Though static_cast<> won't work with them you can still use numeric_cast<> by specializing
// raw_converter as follows:
//
template<> struct raw_converter<Float2IntTraits>
{
  typedef Float2IntTraits::result_type   result_type   ;
  typedef Float2IntTraits::argument_type argument_type ;

  static result_type low_level_convert ( argument_type s ) { return Int((int)s.mV); }
} ;
template<> struct raw_converter<Int2FloatTraits>
{
  typedef Int2FloatTraits::result_type   result_type   ;
  typedef Int2FloatTraits::argument_type argument_type ;

  static result_type low_level_convert ( argument_type s ) { return Float(s.mV); }
} ;

} }

void custom_raw_converter()
{
  Float f (12.34);
  Int   i (12);
  Float fi(12);

  BOOST_CHECK(numeric_cast<Int>  (f).mV == i .mV ) ;
  BOOST_CHECK(numeric_cast<Float>(i).mV == fi.mV ) ;
}

//
// Alterntively, the custom raw_converter classes can be defined non-instrusively
// (not as specializations) and passed along as policies
//
struct Float2IntRawConverter
{
  static Int low_level_convert ( Float const& s ) { return Int((int)s.mV); }
} ;
struct Int2FloatRawConverter
{
  static Float low_level_convert ( Int const& s ) { return Float(s.mV); }
} ;

void custom_raw_converter2()
{
  Float f (12.34);
  Int   i (12);
  Float fi(12);

  typedef converter<Int
                   ,Float
                   ,Float2IntTraits
                   ,void  // By default UDT disable range checking so this won't be used
                   ,void  // Float2Int Rounder won't be used if Int isn't marked as integer via numeric_limits<>
                   ,Float2IntRawConverter
                   >
                   Float2IntConverter ;

  BOOST_CHECK(Float2IntConverter::convert(f).mV == i .mV ) ;
}

int test_main( int, char* [] )
{
  cout << setprecision( numeric_limits<long double>::digits10 ) ;

  simplest_case();
  rounding();
  custom_rounding();
  custom_raw_converter();
  custom_raw_converter2();

  return 0;
}






