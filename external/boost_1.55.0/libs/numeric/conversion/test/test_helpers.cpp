// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


//
// NOTE: This file is intended to be used ONLY by the test files
//       from the Numeric Conversions Library
//
//
#include <cmath>

#include "boost/limits.hpp"
#include "boost/utility.hpp"

#include "boost/test/included/test_exec_monitor.hpp"

// Convenience macros to help with compilers which don't parse
// explicit template function instantiations (MSVC6)
#define MATCH_FNTPL_ARG(t) t const*
#define SET_FNTPL_ARG(t) (static_cast< t const* >(0))

//
// *Minimal* example of a User Defined Numeric Type
//
//
namespace MyUDT
{

template<class T>
struct UDT
{
  typedef T builtin_type ;

  UDT ( T v_ ) : v (v_) {}

  T to_builtin() const { return v ; }

  friend bool operator == ( UDT const& lhs, UDT const& rhs )
    { return lhs.to_builtin() == rhs.to_builtin() ; }

  // NOTE: This operator is *required* by the Numeric Conversion Library
  //       if Turnc<> is used as the Float2IntRounder policy.
  friend bool operator < ( UDT const& lhs, UDT const& rhs )
    { return lhs.to_builtin() < rhs.to_builtin() ; }

  friend std::ostream& operator << ( std::ostream& os, UDT const& n )
    { return os << n.to_builtin() ; }

  T v ;
} ;

typedef UDT<int>    MyInt ;
typedef UDT<double> MyFloat ;

//
// The Float2IntRounder policies *require* a visible 'ceil' or 'floor' math function
// with standard semantics.
// In a conformant compiler, ADL can pick these functions even if they are defined
// within a user namespace, as below.
//
inline MyInt ceil  ( MyInt const& x ) { return x ; }
inline MyInt floor ( MyInt const& x ) { return x ; }

inline MyFloat floor ( MyFloat const& x )
{
#if !defined(BOOST_NO_STDC_NAMESPACE)
  return MyFloat ( std::floor(x.to_builtin()) ) ;
#else
  return MyFloat ( ::floor(x.to_builtin()) ) ;
#endif
}

inline MyFloat ceil ( MyFloat const& x )
{
#if !defined(BOOST_NO_STDC_NAMESPACE)
  return MyFloat ( std::ceil(x.to_builtin()) ) ;
#else
  return MyFloat ( ::ceil(x.to_builtin()) ) ;
#endif
}

} // namespace MyUDT


//
// The Numeric Conversion Library *requires* User Defined Numeric Types
// to properly specialize std::numeric_limits<>
//
namespace std
{

template<>
class numeric_limits<MyUDT::MyInt> : public numeric_limits<int>
{
  public :

    BOOST_STATIC_CONSTANT(bool, is_specialized = false);
} ;

template<>
class numeric_limits<MyUDT::MyFloat> : public numeric_limits<double>
{
  public :

    BOOST_STATIC_CONSTANT(bool, is_specialized = false);
} ;

} // namespace std



//
// The functions floor and ceil defined within namespace MyUDT
// should be found by koenig loopkup, but some compilers don't do it right
// so we inyect them into namespace std so ordinary overload resolution
// can found them.
#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP) || defined(__BORLANDC__) || defined(__GNUC__)
namespace std {
using MyUDT::floor ;
using MyUDT::ceil  ;
} // namespace std
#endif


std::string to_string( bool arg )
{
  return arg ? "true" : "false" ;
}

std::string to_string( ... ) { throw std::runtime_error("to_string() called with wrong type!") ; }

//
// This is used to print 'char' values as numbers instead of characters.
//
template<class T> struct printable_number_type   { typedef T type ; } ;
template<> struct printable_number_type<signed char>   { typedef int type ; } ;
template<> struct printable_number_type<unsigned char> { typedef unsigned type ; } ;
template<> struct printable_number_type<char>          { typedef int type ; } ;

template<class T>
inline
typename printable_number_type<T>::type
printable( T n ) { return n ; }


//
///////////////////////////////////////////////////////////////////////////////////////////////

