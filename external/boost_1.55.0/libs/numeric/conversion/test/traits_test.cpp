// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
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

#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <boost/preprocessor/cat.hpp>

#include <boost/numeric/conversion/conversion_traits.hpp>
#include <boost/numeric/conversion/int_float_mixture.hpp>
#include <boost/numeric/conversion/sign_mixture.hpp>
#include <boost/numeric/conversion/udt_builtin_mixture.hpp>
#include <boost/numeric/conversion/is_subranged.hpp>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "test_helpers.cpp"
#include "test_helpers2.cpp"

using namespace std ;
using namespace boost ;
using namespace numeric;
using namespace MyUDT ;

// These helpers are used by generate_expected_traits<T,S>.
// Unlike the similar helpers in the implementation, they are specialized by extension.
//
template<class T, class S> struct my_is_subranged ;
template<class T, class S> struct my_is_trivial   ;
template<class T, class S> struct my_int_float_mixture ;
template<class T, class S> struct my_sign_mixture ;
template<class T, class S> struct my_udt_builtin_mixture ;

// This macro is used to define the properties of each conversion between
// the builtin arithmetric types
//
// It defines the specialization of the helper traits used by 'generate_expected_traits'
//
#define DEFINE_CONVERSION(Target,Source,Trivial,Mixture,SignMixture,UdtMixture,SubRanged) \
                                                          \
        template<> struct my_is_subranged<Target,Source>  \
          { typedef mpl::bool_< (SubRanged) > type ; } ; \
                                                         \
        template<> struct my_is_trivial<Target,Source>   \
          { typedef mpl::bool_< (Trivial) > type ; } ;  \
                                                                                                 \
        template<> struct my_int_float_mixture<Target,Source>                                    \
          { typedef mpl::integral_c<boost::numeric::int_float_mixture_enum, (Mixture) > type ; } ;    \
                                                                                                 \
        template<> struct my_sign_mixture<Target,Source>                                         \
          { typedef mpl::integral_c<boost::numeric::sign_mixture_enum, (SignMixture) > type ; } ;     \
                                                                                                 \
        template<> struct my_udt_builtin_mixture<Target,Source>                                  \
          { typedef mpl::integral_c<boost::numeric::udt_builtin_mixture_enum, (UdtMixture) > type ; }


#define cSubRanged true
#define cTrivial   true

// The following test assumes a specific relation between the sizes of the types being used;
// therefore, use specific fixed-width types instead built-in types directly.

// NOTE --> TARGET,SOURCE
//
DEFINE_CONVERSION(boost::uint8_t  , boost::uint8_t,  cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , boost::uint8_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , boost::uint8_t, !cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , boost::uint8_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , boost::uint8_t, !cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , boost::uint8_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(float           , boost::uint8_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(double          , boost::uint8_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(long double     , boost::uint8_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(MyInt           , boost::uint8_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , boost::uint8_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , boost::int8_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , boost::int8_t,  cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , boost::int8_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , boost::int8_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , boost::int8_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , boost::int8_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(float           , boost::int8_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(double          , boost::int8_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(long double     , boost::int8_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(MyInt           , boost::int8_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , boost::int8_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , boost::uint16_t, !cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , boost::uint16_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , boost::uint16_t,  cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , boost::uint16_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , boost::uint16_t, !cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , boost::uint16_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(float           , boost::uint16_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(double          , boost::uint16_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(long double     , boost::uint16_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(MyInt           , boost::uint16_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , boost::uint16_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , boost::int16_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , boost::int16_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , boost::int16_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , boost::int16_t,  cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , boost::int16_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , boost::int16_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(float           , boost::int16_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(double          , boost::int16_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(long double     , boost::int16_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(MyInt           , boost::int16_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , boost::int16_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , boost::uint32_t, !cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , boost::uint32_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , boost::uint32_t, !cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , boost::uint32_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , boost::uint32_t,  cTrivial, integral_to_integral, unsigned_to_unsigned, builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , boost::uint32_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(float           , boost::uint32_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(double          , boost::uint32_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(long double     , boost::uint32_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(MyInt           , boost::uint32_t, !cTrivial, integral_to_integral, unsigned_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , boost::uint32_t, !cTrivial, integral_to_float   , unsigned_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , boost::int32_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , boost::int32_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , boost::int32_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , boost::int32_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , boost::int32_t, !cTrivial, integral_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , boost::int32_t,  cTrivial, integral_to_integral, signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(float           , boost::int32_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(double          , boost::int32_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(long double     , boost::int32_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(MyInt           , boost::int32_t, !cTrivial, integral_to_integral, signed_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , boost::int32_t, !cTrivial, integral_to_float   , signed_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , float, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , float, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , float, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , float, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , float, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , float, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(float           , float,  cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(double          , float, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, ( sizeof(float) > sizeof(double) ) );
DEFINE_CONVERSION(long double     , float, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, ( sizeof(float) > sizeof(long double) ) );
DEFINE_CONVERSION(MyInt           , float, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , float, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , double, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , double, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , double, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(float           , double, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, ( sizeof(double) > sizeof(float) ) );
DEFINE_CONVERSION(double          , double,  cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(long double     , double, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, ( sizeof(double) > sizeof(long double) ) );
DEFINE_CONVERSION(MyInt           , double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , double, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , long double, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , long double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , long double, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , long double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , long double, !cTrivial, float_to_integral, signed_to_unsigned, builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , long double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_builtin,  cSubRanged );
DEFINE_CONVERSION(float           , long double, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, ( sizeof(long double) > sizeof(float) ) );
DEFINE_CONVERSION(double          , long double, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, ( sizeof(long double) > sizeof(double) ) );
DEFINE_CONVERSION(long double     , long double,  cTrivial, float_to_float   , signed_to_signed  , builtin_to_builtin, !cSubRanged );
DEFINE_CONVERSION(MyInt           , long double, !cTrivial, float_to_integral, signed_to_signed  , builtin_to_udt    , !cSubRanged );
DEFINE_CONVERSION(MyFloat         , long double, !cTrivial, float_to_float   , signed_to_signed  , builtin_to_udt    , !cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , MyInt, !cTrivial, integral_to_integral, signed_to_unsigned, udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , MyInt, !cTrivial, integral_to_integral, signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , MyInt, !cTrivial, integral_to_integral, signed_to_unsigned, udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , MyInt, !cTrivial, integral_to_integral, signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , MyInt, !cTrivial, integral_to_integral, signed_to_unsigned, udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , MyInt, !cTrivial, integral_to_integral, signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(float           , MyInt, !cTrivial, integral_to_float   , signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(double          , MyInt, !cTrivial, integral_to_float   , signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(long double     , MyInt, !cTrivial, integral_to_float   , signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(MyInt           , MyInt,  cTrivial, integral_to_integral, signed_to_signed  , udt_to_udt     ,!cSubRanged );
DEFINE_CONVERSION(MyFloat         , MyInt, !cTrivial, integral_to_float   , signed_to_signed  , udt_to_udt     ,!cSubRanged );

DEFINE_CONVERSION(boost::uint8_t  , MyFloat, !cTrivial, float_to_integral, signed_to_unsigned, udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::int8_t   , MyFloat, !cTrivial, float_to_integral, signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::uint16_t , MyFloat, !cTrivial, float_to_integral, signed_to_unsigned, udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::int16_t  , MyFloat, !cTrivial, float_to_integral, signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::uint32_t , MyFloat, !cTrivial, float_to_integral, signed_to_unsigned, udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(boost::int32_t  , MyFloat, !cTrivial, float_to_integral, signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(float           , MyFloat, !cTrivial, float_to_float   , signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(double          , MyFloat, !cTrivial, float_to_float   , signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(long double     , MyFloat, !cTrivial, float_to_float   , signed_to_signed  , udt_to_builtin , cSubRanged );
DEFINE_CONVERSION(MyInt           , MyFloat, !cTrivial, float_to_integral, signed_to_signed  , udt_to_udt     ,!cSubRanged );
DEFINE_CONVERSION(MyFloat         , MyFloat,  cTrivial, float_to_float   , signed_to_signed  , udt_to_udt     ,!cSubRanged );

//
// The test is performed by comparing each field of
//   boost::numeric::conversion_traits<T,S>
// with the fields of
//   expected_traits<T,S>
// which is a traits class constructed explicitely for each combination
// of the built-in arithmetic types.
//
template<class T,
         class S,
         class Supertype,
         class Subtype,
         class Subranged,
         class Trivial
        >
struct expected_traits
{
  typedef typename my_int_float_mixture   <T,S>::type int_float_mixture ;
  typedef typename my_sign_mixture        <T,S>::type sign_mixture ;
  typedef typename my_udt_builtin_mixture <T,S>::type udt_builtin_mixture ;

  typedef Subranged subranged ;
  typedef Trivial   trivial   ;
  typedef Supertype supertype ;
  typedef Subtype   subtype   ;
} ;

// This is used by the test engine to generate a expected_traits from T and S.
//
template<class T, class S>
struct generate_expected_traits
{
  typedef expected_traits<T, S, T, S, mpl::false_, mpl::true_  > trivial ;
  typedef expected_traits<T, S, S, T, mpl::true_ , mpl::false_ > subranged ;
  typedef expected_traits<T, S, T, S, mpl::false_, mpl::false_ > non_subranged ;

  typedef typename my_is_subranged<T,S>::type IsSubranged ;
  typedef typename my_is_trivial  <T,S>::type IsTrivial   ;

  typedef typename mpl::if_<IsSubranged,subranged,non_subranged>::type non_trivial ;

  typedef typename mpl::if_<IsTrivial,trivial,non_trivial>::type  type ;
} ;

// This macro generates the code that compares a non-type field
// in boost::numeric::conversion_traits<> with its corresponding field
// in expected_traits<>
//

#define TEST_VALUE_FIELD(Name) \
        typedef typename traits::Name   BOOST_PP_CAT(t,Name) ; \
        typedef typename expected::Name BOOST_PP_CAT(x,Name) ; \
        BOOST_CHECK_MESSAGE ( ( BOOST_PP_CAT(t,Name)::value == BOOST_PP_CAT(x,Name)::value ) , \
                              "conversion_traits<" << typeid(T).name() << "," << typeid(S).name() \
                              << ">::" << #Name << " = " << to_string(BOOST_PP_CAT(t,Name)::value) \
                              << ". Expected: "  << to_string(BOOST_PP_CAT(x,Name)::value) \
                            ) ;

// This macro generates the code that compares a type field
// in numeric::conversion_traits<> with its corresponding field
// in expected_traits<>
//
#define TEST_TYPE_FIELD(Name) \
        typedef typename traits::Name   BOOST_PP_CAT(t,Name) ; \
        typedef typename expected::Name BOOST_PP_CAT(x,Name) ; \
        BOOST_CHECK_MESSAGE ( ( typeid(BOOST_PP_CAT(t,Name)) == typeid(BOOST_PP_CAT(x,Name)) ) , \
                              "conversion_traits<" << typeid(T).name() << "," << typeid(S).name() \
                              << ">::" << #Name << " = " <<  typeid(BOOST_PP_CAT(t,Name)).name() \
                              << ". Expected: "  << typeid(BOOST_PP_CAT(x,Name)).name() \
                            ) ;

//
// Test core.
// Compares each field of boost::numeric::conversion_traits<T,S>
// with the corresponding field of expected_traits<T,S>
//
template<class T, class S>
void test_traits_base(  MATCH_FNTPL_ARG(T),  MATCH_FNTPL_ARG(S) )
{
  typedef boost::numeric::conversion_traits<T,S> traits ;
  typedef typename generate_expected_traits<T,S>::type expected ;

  TEST_VALUE_FIELD(int_float_mixture) ;
  TEST_VALUE_FIELD(sign_mixture) ;
  TEST_VALUE_FIELD(udt_builtin_mixture) ;
  TEST_VALUE_FIELD(subranged) ;
  TEST_VALUE_FIELD(trivial) ;
  TEST_TYPE_FIELD (supertype) ;
  TEST_TYPE_FIELD (subtype) ;
}


template<class S>
void test_traits_from(  MATCH_FNTPL_ARG(S) )
{
  test_traits_base( SET_FNTPL_ARG(boost::uint8_t)  ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(boost::int8_t)   ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(boost::uint16_t) ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(boost::int16_t)  ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(boost::uint32_t) ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(boost::int32_t)  ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(float)           ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(double)          ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(long double)     ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(MyInt)           ,SET_FNTPL_ARG(S) );
  test_traits_base( SET_FNTPL_ARG(MyFloat)         ,SET_FNTPL_ARG(S) );
}

void test_traits()
{
  test_traits_from( SET_FNTPL_ARG(boost::uint8_t)  );
  test_traits_from( SET_FNTPL_ARG(boost::int8_t)   );
  test_traits_from( SET_FNTPL_ARG(boost::uint16_t) );
  test_traits_from( SET_FNTPL_ARG(boost::int16_t)  );
  test_traits_from( SET_FNTPL_ARG(boost::uint32_t) );
  test_traits_from( SET_FNTPL_ARG(boost::int32_t)  );
  test_traits_from( SET_FNTPL_ARG(float)           );
  test_traits_from( SET_FNTPL_ARG(double)          );
  test_traits_from( SET_FNTPL_ARG(long double)     );
  test_traits_from( SET_FNTPL_ARG(MyInt)           );
  test_traits_from( SET_FNTPL_ARG(MyFloat)         );
}

int test_main( int, char * [])
{
  std::cout << std::setprecision( std::numeric_limits<long double>::digits10 ) ;
  
  test_traits();

  return 0;
}
//---------------------------------------------------------------------------

