// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/lib/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//
#include<iostream>
#include<stdexcept>
#include<string>

#define BOOST_ENABLE_ASSERT_HANDLER

#include "boost/optional.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/none.hpp"

#include "boost/test/minimal.hpp"

#include "optional_test_common.cpp"

template<class T>
inline void check_ref_uninitialized_const ( optional<T&> const& opt )
{
#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt == 0 ) ;
#endif
  BOOST_CHECK( !opt ) ;
}
template<class T>
inline void check_ref_uninitialized ( optional<T&>& opt )
{
#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt == 0 ) ;
#endif
  BOOST_CHECK( !opt ) ;

  check_ref_uninitialized_const(opt);
}

template<class T>
inline void check_ref_initialized_const ( optional<T&> const& opt )
{
  BOOST_CHECK( opt ) ;

#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt != 0 ) ;
#endif

  BOOST_CHECK ( !!opt ) ;
}

template<class T>
inline void check_ref_initialized ( optional<T&>& opt )
{
  BOOST_CHECK( opt ) ;

#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt != 0 ) ;
#endif

  BOOST_CHECK ( !!opt ) ;

  check_ref_initialized_const(opt);
}

template<class T>
inline void check_ref_value_const ( optional<T&> const& opt, T const& v, T const& z )
{
  BOOST_CHECK( *opt == v ) ;
  BOOST_CHECK( *opt != z ) ;
  BOOST_CHECK( opt.get() == v ) ;
  BOOST_CHECK( opt.get() != z ) ;
}

template<class T>
inline void check_ref_value ( optional<T&>& opt, T const& v, T const& z )
{
  BOOST_CHECK( *opt == v ) ;
  BOOST_CHECK( *opt != z ) ;
  BOOST_CHECK( opt.get() == v ) ;
  BOOST_CHECK( opt.get() != z ) ;

  check_ref_value_const(opt,v,z);
}

//
// Basic test.
// Check ordinary functionality:
//   Initialization, assignment, comparison and value-accessing.
//
template<class T>
void test_basics( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION  );

  T z(0);

  T original_a(1);

  T a(1);

  T b(2);

  T c(10);

  T& aref = a ;
  T& bref = b ;

  // Default construction.
  // 'def' state is Uninitialized.
  // T::T() is not called
  optional<T&> def ;
  check_ref_uninitialized(def);

  // Direct initialization.
  // 'oa' state is Initialized and binds to 'a'
  // T::T( T const& x ) is NOT used becasue the optional holds a reference.
  set_pending_copy( ARG(T) ) ;
  optional<T&> oa ( aref ) ;
  check_is_pending_copy( ARG(T) );
  check_ref_initialized(oa);
  check_ref_value(oa,a,z);
  *oa = b ; // changes the value of 'a' through the reference
  BOOST_CHECK( a == b ) ;


  // Copy initialization.
  // T::T ( T const& x ) is NOT used becasue the optional holds a reference.
  set_pending_copy( ARG(T) ) ;
  optional<T&>  const oa2 ( oa ) ;
  check_is_pending_copy( ARG(T) ) ;
  check_ref_initialized_const(oa2);
  check_ref_value_const(oa2,a,z);
  *oa2 = original_a ; // restores the value of 'a' through the reference
  BOOST_CHECK( a == original_a ) ;

  optional<T&> ob ;

  // Value-Assignment upon Uninitialized optional.
  // T::T ( T const& x ) is NOT used becasue the optional holds a reference.
  set_pending_copy( ARG(T) ) ;
  ob = a ; // Binds ob to a temporary non-const refererence to 'a'
  check_is_pending_copy( ARG(T) ) ;
  check_ref_initialized(ob);
  check_ref_value(ob,a,z);
  a = c;
  check_ref_value(ob,a,z);

  // Value-Assignment upon Initialized optional.
  // T::operator= ( T const& x ) is used.
  set_pending_assign( ARG(T) ) ;
  ob = b ; // Rebinds 'ob' to 'b' (without changing 'a')
  check_is_pending_assign( ARG(T) ) ;
  check_ref_initialized(ob);
  check_ref_value(ob,b,z);
  BOOST_CHECK(a == c); // From a=c in previous test
  b = c;
  check_ref_value(ob,b,z);


  // Assignment initialization.
  // T::T ( T const& x ) is NOT used becasue the optional holds a reference.
  set_pending_copy( ARG(T) ) ;
  optional<T&> const oa3 = b ;
  check_is_pending_copy( ARG(T) ) ;
  check_ref_initialized_const(oa3);
  check_ref_value_const(oa3,b,z);


  // Assignment
  // T::operator=( T const& x ) is used.
  set_pending_assign( ARG(T) ) ;
  oa = ob ; // Rebinds 'a' to 'b'
  check_is_pending_assign( ARG(T) ) ;
  check_ref_initialized(oa);
  a = original_a ;
  check_ref_value(oa,b,z);

  // Uninitializing Assignment upon Initialized Optional
  // T::~T() is NOT used becasue the optional holds a reference.
  set_pending_dtor( ARG(T) ) ;
  set_pending_copy( ARG(T) ) ;
  oa = def ;
  check_is_pending_dtor( ARG(T) ) ;
  check_is_pending_copy( ARG(T) ) ;
  check_ref_uninitialized(oa);

  // Uninitializing Assignment upon Uninitialized Optional
  // (Dtor is not called this time)
  set_pending_dtor( ARG(T) ) ;
  set_pending_copy( ARG(T) ) ;
  oa = def ;
  check_is_pending_dtor( ARG(T) ) ;
  check_is_pending_copy( ARG(T) ) ;
  check_ref_uninitialized(oa);


  // Deinitialization of Initialized Optional
  // T::~T() is NOT used becasue the optional holds a reference.
  set_pending_dtor( ARG(T) ) ;
  ob.reset();
  check_is_pending_dtor( ARG(T) ) ;
  check_ref_uninitialized(ob);

  // Deinitialization of Uninitialized Optional
  // T::~T() is not called this time
  set_pending_dtor( ARG(T) ) ;
  ob.reset();
  check_is_pending_dtor( ARG(T) ) ;
  check_ref_uninitialized(ob);
}

//
// This verifies relational operators.
//
template<class T>
void test_relops( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  reset_throw_on_copy( ARG(T) ) ;

  T v0(18);
  T v1(19);
  T v2(19);

  optional<T&> def0 ;
  optional<T&> def1 ;
  optional<T&> opt0(v0);
  optional<T&> opt1(v1);
  optional<T&> opt2(v2);

  // Check identity
  BOOST_CHECK ( def0 == def0 ) ;
  BOOST_CHECK ( opt0 == opt0 ) ;
  BOOST_CHECK ( !(def0 != def0) ) ;
  BOOST_CHECK ( !(opt0 != opt0) ) ;

  // Check when both are uininitalized.
  BOOST_CHECK (   def0 == def1  ) ; // both uninitialized compare equal
  BOOST_CHECK ( !(def0 <  def1) ) ; // uninitialized is never less    than uninitialized
  BOOST_CHECK ( !(def0 >  def1) ) ; // uninitialized is never greater than uninitialized
  BOOST_CHECK ( !(def0 != def1) ) ;
  BOOST_CHECK (   def0 <= def1  ) ;
  BOOST_CHECK (   def0 >= def1  ) ;

  // Check when only lhs is uninitialized.
  BOOST_CHECK (   def0 != opt0  ) ; // uninitialized is never equal to initialized
  BOOST_CHECK ( !(def0 == opt0) ) ;
  BOOST_CHECK (   def0 <  opt0  ) ; // uninitialized is always less than initialized
  BOOST_CHECK ( !(def0 >  opt0) ) ;
  BOOST_CHECK (   def0 <= opt0  ) ;
  BOOST_CHECK ( !(def0 >= opt0) ) ;

  // Check when only rhs is uninitialized.
  BOOST_CHECK (   opt0 != def0  ) ; // initialized is never equal to uninitialized
  BOOST_CHECK ( !(opt0 == def0) ) ;
  BOOST_CHECK ( !(opt0 <  def0) ) ; // initialized is never less than uninitialized
  BOOST_CHECK (   opt0 >  def0  ) ;
  BOOST_CHECK ( !(opt0 <= def0) ) ;
  BOOST_CHECK (   opt0 >= opt0  ) ;

  // If both are initialized, values are compared
  BOOST_CHECK ( opt0 != opt1 ) ;
  BOOST_CHECK ( opt1 == opt2 ) ;
  BOOST_CHECK ( opt0 <  opt1 ) ;
  BOOST_CHECK ( opt1 >  opt0 ) ;
  BOOST_CHECK ( opt1 <= opt2 ) ;
  BOOST_CHECK ( opt1 >= opt0 ) ;
}

template<class T>
void test_none( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  using boost::none ;

  T a(1234);

  optional<T&> def0 ;
  optional<T&> def1(none) ;
  optional<T&> non_def(a) ;

  BOOST_CHECK ( def0    == none ) ;
  BOOST_CHECK ( non_def != none ) ;
  BOOST_CHECK ( !def1           ) ;

  non_def = none ;
  BOOST_CHECK ( !non_def ) ;
}

template<class T>
void test_arrow( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T a(1234);

  optional<T&>        oa(a) ;
  optional<T&> const coa(a) ;
  
  BOOST_CHECK ( coa->V() == 1234 ) ;
  
  oa->V() = 4321 ;
  
  BOOST_CHECK ( a.V() = 4321 ) ;
}

void test_with_builtin_types()
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  test_basics( ARG(double) );
  test_relops( ARG(double) ) ;
  test_none  ( ARG(double) ) ;
}

void test_with_class_type()
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  test_basics( ARG(X) );
  test_relops( ARG(X) ) ;
  test_none  ( ARG(X) ) ;
  test_arrow ( ARG(X) ) ;

  BOOST_CHECK ( X::count == 0 ) ;
}

void test_binding()
{
  int i = 0 ;
  optional<int&> ori1 = i ;
  BOOST_CHECK(  &(*ori1) == &i ) ;

  optional<int&> ori2(i) ;
  BOOST_CHECK(  &(*ori2) == &i ) ;

  int const ci = 0 ;
  optional<int const&> orci1 = ci ;
  BOOST_CHECK(  &(*orci1) == &ci ) ;

  optional<int const&> orci2(ci) ;
  BOOST_CHECK(  &(*orci2) == &ci ) ;
}

int test_main( int, char* [] )
{
  try
  {
    test_with_class_type();
    test_with_builtin_types();
    test_binding();
  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


