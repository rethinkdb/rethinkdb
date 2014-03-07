// Copyright (C) 2003, 2008 Fernando Luis Cacciola Carballal.
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
// Revisions:
// 12 May 2008 (added more swap tests)
//
#include<iostream>
#include<stdexcept>
#include<string>

#define BOOST_ENABLE_ASSERT_HANDLER

#include "boost/bind/apply.hpp" // Included just to test proper interaction with boost::apply<> as reported by Daniel Wallin
#include "boost/mpl/bool.hpp"
#include "boost/mpl/bool_fwd.hpp"  // For mpl::true_ and mpl::false_

#include "boost/optional/optional.hpp"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "boost/none.hpp"

#include "boost/test/minimal.hpp"

#include "optional_test_common.cpp"

void test_implicit_construction ( optional<double> opt, double v, double z )
{
  check_value(opt,v,z);
}

void test_implicit_construction ( optional<X> opt, X const& v, X const& z )
{
  check_value(opt,v,z);
}

void test_default_implicit_construction ( double, optional<double> opt )
{
  BOOST_CHECK(!opt);
}

void test_default_implicit_construction ( X const&, optional<X> opt )
{
  BOOST_CHECK(!opt);
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

  T a(1);

  // Default construction.
  // 'def' state is Uninitialized.
  // T::T() is not called (and it is not even defined)
  optional<T> def ;
  check_uninitialized(def);

  // Implicit construction
  // The first parameter is implicitely converted to optional<T>(a);
  test_implicit_construction(a,a,z);

  // Direct initialization.
  // 'oa' state is Initialized with 'a'
  // T::T( T const& x ) is used.
  set_pending_copy( ARG(T) ) ;
  optional<T> oa ( a ) ;
  check_is_not_pending_copy( ARG(T) );
  check_initialized(oa);
  check_value(oa,a,z);

  T b(2);

  optional<T> ob ;

  // Value-Assignment upon Uninitialized optional.
  // T::T( T const& x ) is used.
  set_pending_copy( ARG(T) ) ;
  ob = a ;
  check_is_not_pending_copy( ARG(T) ) ;
  check_initialized(ob);
  check_value(ob,a,z);

  // Value-Assignment upon Initialized optional.
  // T::operator=( T const& x ) is used
  set_pending_assign( ARG(T) ) ;
  set_pending_copy  ( ARG(T) ) ;
  set_pending_dtor  ( ARG(T) ) ;
  ob = b ;
  check_is_not_pending_assign( ARG(T) ) ;
  check_is_pending_copy      ( ARG(T) ) ;
  check_is_pending_dtor      ( ARG(T) ) ;
  check_initialized(ob);
  check_value(ob,b,z);

  // Assignment initialization.
  // T::T ( T const& x ) is used to copy new value.
  set_pending_copy( ARG(T) ) ;
  optional<T> const oa2 ( oa ) ;
  check_is_not_pending_copy( ARG(T) ) ;
  check_initialized_const(oa2);
  check_value_const(oa2,a,z);

  // Assignment
  // T::operator= ( T const& x ) is used to copy new value.
  set_pending_assign( ARG(T) ) ;
  oa = ob ;
  check_is_not_pending_assign( ARG(T) ) ;
  check_initialized(oa);
  check_value(oa,b,z);

  // Uninitializing Assignment upon Initialized Optional
  // T::~T() is used to destroy previous value in oa.
  set_pending_dtor( ARG(T) ) ;
  set_pending_copy( ARG(T) ) ;
  oa = def ;
  check_is_not_pending_dtor( ARG(T) ) ;
  check_is_pending_copy    ( ARG(T) ) ;
  check_uninitialized(oa);

  // Uninitializing Assignment upon Uninitialized Optional
  // (Dtor is not called this time)
  set_pending_dtor( ARG(T) ) ;
  set_pending_copy( ARG(T) ) ;
  oa = def ;
  check_is_pending_dtor( ARG(T) ) ;
  check_is_pending_copy( ARG(T) ) ;
  check_uninitialized(oa);

  // Deinitialization of Initialized Optional
  // T::~T() is used to destroy previous value in ob.
  set_pending_dtor( ARG(T) ) ;
  ob.reset();
  check_is_not_pending_dtor( ARG(T) ) ;
  check_uninitialized(ob);

  // Deinitialization of Uninitialized Optional
  // (Dtor is not called this time)
  set_pending_dtor( ARG(T) ) ;
  ob.reset();
  check_is_pending_dtor( ARG(T) ) ;
  check_uninitialized(ob);

}

template<class T>
void test_conditional_ctor_and_get_valur_or ( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION  );

  T a(321);

  T z(123);

  optional<T> const cdef0(false,a);

  optional<T> def0(false,a);
  optional<T> def1 = boost::make_optional(false,a); //  T is not within boost so ADL won't find make_optional unqualified
  check_uninitialized(def0);
  check_uninitialized(def1);

  optional<T> const co0(true,a);

  optional<T> o0(true,a);
  optional<T> o1 = boost::make_optional(true,a); //  T is not within boost so ADL won't find make_optional unqualified

  check_initialized(o0);
  check_initialized(o1);
  check_value(o0,a,z);
  check_value(o1,a,z);

  T b = def0.get_value_or(z);
  BOOST_CHECK( b == z ) ;

  b = get_optional_value_or(def0,z);
  BOOST_CHECK( b == z ) ;

  b = o0.get_value_or(z);
  BOOST_CHECK( b == a ) ;

  b = get_optional_value_or(o0,z);
  BOOST_CHECK( b == a ) ;


  T const& crz = z ;
  T&        rz = z ;

  T const& crzz = def0.get_value_or(crz);
  BOOST_CHECK( crzz == crz ) ;

  T& rzz = def0.get_value_or(rz);
  BOOST_CHECK( rzz == rz ) ;

  T const& crzzz = get_optional_value_or(cdef0,crz);
  BOOST_CHECK( crzzz == crz ) ;

  T& rzzz = get_optional_value_or(def0,rz);
  BOOST_CHECK( rzzz == rz ) ;

  T const& crb = o0.get_value_or(crz);
  BOOST_CHECK( crb == a ) ;

  T& rb = o0.get_value_or(rz);
  BOOST_CHECK( rb == b ) ;

  T const& crbb = get_optional_value_or(co0,crz);
  BOOST_CHECK( crbb == b ) ;

  T const& crbbb = get_optional_value_or(o0,crz);
  BOOST_CHECK( crbbb == b ) ;

  T& rbb = get_optional_value_or(o0,rz);
  BOOST_CHECK( rbb == b ) ;

  T& ra = a ;

  optional<T&> defref(false,ra);
  BOOST_CHECK(!defref);

  optional<T&> ref(true,ra);
  BOOST_CHECK(!!ref);

  a = T(432);

  BOOST_CHECK( *ref == a ) ;

  T& r1 = defref.get_value_or(z);
  BOOST_CHECK( r1 == z ) ;

  T& r2 = ref.get_value_or(z);
  BOOST_CHECK( r2 == a ) ;
}

//
// Test Direct Value Manipulation
//
template<class T>
void test_direct_value_manip( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T x(3);

  optional<T> const c_opt0(x) ;
  optional<T>         opt0(x);

  BOOST_CHECK( c_opt0.get().V() == x.V() ) ;
  BOOST_CHECK(   opt0.get().V() == x.V() ) ;

  BOOST_CHECK( c_opt0->V() == x.V() ) ;
  BOOST_CHECK(   opt0->V() == x.V() ) ;

  BOOST_CHECK( (*c_opt0).V() == x.V() ) ;
  BOOST_CHECK( (*  opt0).V() == x.V() ) ;

  T y(4);
  opt0 = y ;
  BOOST_CHECK( get(opt0).V() == y.V() ) ;
}

//
// Test Uninitialized access assert
//
template<class T>
void test_uninitialized_access( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  optional<T> def ;

  bool passed = false ;
  try
  {
    // This should throw because 'def' is uninitialized
    T const& n = def.get() ;
    unused_variable(n);
    passed = true ;
  }
  catch (...) {}
  BOOST_CHECK(!passed);

  passed = false ;
  try
  {
    // This should throw because 'def' is uninitialized
    T const& n = *def ;
    unused_variable(n);
    passed = true ;
  }
  catch (...) {}
  BOOST_CHECK(!passed);

  passed = false ;
  try
  {
    T v(5) ;
    unused_variable(v);
    // This should throw because 'def' is uninitialized
    *def = v ;
    passed = true ;
  }
  catch (...) {}
  BOOST_CHECK(!passed);

  passed = false ;
  try
  {
    // This should throw because 'def' is uninitialized
    T v = *(def.operator->()) ;
    unused_variable(v);
    passed = true ;
  }
  catch (...) {}
  BOOST_CHECK(!passed);
}

#if BOOST_WORKAROUND( BOOST_INTEL_CXX_VERSION, <= 700) // Intel C++ 7.0
void prevent_buggy_optimization( bool v ) {}
#endif

//
// Test Direct Initialization of optional for a T with throwing copy-ctor.
//
template<class T>
void test_throwing_direct_init( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T a(6);

  int count = get_instance_count( ARG(T) ) ;

  set_throw_on_copy( ARG(T) ) ;

  bool passed = false ;
  try
  {
    // This should:
    //   Attempt to copy construct 'a' and throw.
    // 'opt' won't be constructed.
    set_pending_copy( ARG(T) ) ;

#if BOOST_WORKAROUND( BOOST_INTEL_CXX_VERSION, <= 700) // Intel C++ 7.0
    // Intel C++ 7.0 specific:
    //    For some reason, when "check_is_not_pending_copy",
    //    after the exception block is reached,
    //    X::pending_copy==true even though X's copy ctor set it to false.
    //    I guessed there is some sort of optimization bug,
    //    and it seems to be the since the following additional line just
    //    solves the problem (!?)
    prevent_buggy_optimization(X::pending_copy);
#endif

    optional<T> opt(a) ;
    passed = true ;
  }
  catch ( ... ){}

  BOOST_CHECK(!passed);
  check_is_not_pending_copy( ARG(T) );
  check_instance_count(count, ARG(T) );

  reset_throw_on_copy( ARG(T) ) ;

}

//
// Test Value Assignment to an Uninitialized optional for a T with a throwing copy-ctor
//
template<class T>
void test_throwing_val_assign_on_uninitialized( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T a(7);

  int count = get_instance_count( ARG(T) ) ;

  set_throw_on_copy( ARG(T) ) ;

  optional<T> opt ;

  bool passed = false ;
  try
  {
    // This should:
    //   Attempt to copy construct 'a' and throw.
    //   opt should be left uninitialized.
    set_pending_copy( ARG(T) ) ;
    opt.reset( a );
    passed = true ;
  }
  catch ( ... ) {}

  BOOST_CHECK(!passed);

  check_is_not_pending_copy( ARG(T) );
  check_instance_count(count, ARG(T) );
  check_uninitialized(opt);

  reset_throw_on_copy( ARG(T) ) ;
}

//
// Test Value Reset on an Initialized optional for a T with a throwing copy-ctor
//
template<class T>
void test_throwing_val_assign_on_initialized( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T z(0);
  T a(8);
  T b(9);
  T x(-1);

  int count = get_instance_count( ARG(T) ) ;

  optional<T> opt ( b ) ;
  ++ count ;

  check_instance_count(count, ARG(T) );

  check_value(opt,b,z);

  set_throw_on_assign( ARG(T) ) ;

  bool passed = false ;
  try
  {
    // This should:
    //   Attempt to assign 'a' and throw.
    //   opt is kept initialized but its value not neccesarily fully assigned
    //   (in this test, incompletely assigned is flaged with the value -1 being set)
    set_pending_assign( ARG(T) ) ;
    opt.reset ( a ) ;
    passed = true ;
  }
  catch ( ... ) {}

  BOOST_CHECK(!passed);

  check_is_not_pending_assign( ARG(T) );
  check_instance_count(count, ARG(T) );
  check_initialized(opt);
  check_value(opt,x,z);

  reset_throw_on_assign ( ARG(T) ) ;
}

//
// Test Copy Initialization from an Initialized optional for a T with a throwing copy-ctor
//
template<class T>
void test_throwing_copy_initialization( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T z(0);
  T a(10);

  optional<T> opt (a);

  int count = get_instance_count( ARG(T) ) ;

  set_throw_on_copy( ARG(T) ) ;

  bool passed = false ;
  try
  {
    // This should:
    //   Attempt to copy construct 'opt' and throw.
    //   opt1 won't be constructed.
    set_pending_copy( ARG(T) ) ;
    optional<T> opt1 = opt ;
    passed = true ;
  }
  catch ( ... ) {}

  BOOST_CHECK(!passed);

  check_is_not_pending_copy( ARG(T) );
  check_instance_count(count, ARG(T) );

  // Nothing should have happened to the source optional.
  check_initialized(opt);
  check_value(opt,a,z);

  reset_throw_on_copy( ARG(T) ) ;
}

//
// Test Assignment to an Uninitialized optional from an Initialized optional
// for a T with a throwing copy-ctor
//
template<class T>
void test_throwing_assign_to_uninitialized( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T z(0);
  T a(11);

  optional<T> opt0 ;
  optional<T> opt1(a) ;

  int count = get_instance_count( ARG(T) ) ;

  set_throw_on_copy( ARG(T) ) ;

  bool passed = false ;
  try
  {
    // This should:
    //   Attempt to copy construct 'opt1.value()' into opt0 and throw.
    //   opt0 should be left uninitialized.
    set_pending_copy( ARG(T) ) ;
    opt0 = opt1 ;
    passed = true ;
  }
  catch ( ... ) {}

  BOOST_CHECK(!passed);

  check_is_not_pending_copy( ARG(T) );
  check_instance_count(count, ARG(T) );
  check_uninitialized(opt0);

  reset_throw_on_copy( ARG(T) ) ;
}

//
// Test Assignment to an Initialized optional from an Initialized optional
// for a T with a throwing copy-ctor
//
template<class T>
void test_throwing_assign_to_initialized( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T z(0);
  T a(12);
  T b(13);
  T x(-1);

  optional<T> opt0(a) ;
  optional<T> opt1(b) ;

  int count = get_instance_count( ARG(T) ) ;

  set_throw_on_assign( ARG(T) ) ;

  bool passed = false ;
  try
  {
    // This should:
    //   Attempt to copy construct 'opt1.value()' into opt0 and throw.
    //   opt0 is kept initialized but its value not neccesarily fully assigned
    //   (in this test, incompletely assigned is flaged with the value -1 being set)
    set_pending_assign( ARG(T) ) ;
    opt0 = opt1 ;
    passed = true ;
  }
  catch ( ... ) {}

  BOOST_CHECK(!passed);

  // opt0 was left uninitialized
  check_is_not_pending_assign( ARG(T) );
  check_instance_count(count, ARG(T) );
  check_initialized(opt0);
  check_value(opt0,x,z);

  reset_throw_on_assign( ARG(T) ) ;
}

//
// Test swap in a no-throwing case
//
template<class T>
void test_no_throwing_swap( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T z(0);
  T a(14);
  T b(15);

  optional<T> def0 ;
  optional<T> def1 ;
  optional<T> opt0(a) ;
  optional<T> opt1(b) ;

  int count = get_instance_count( ARG(T) ) ;

  swap(def0,def1);
  check_uninitialized(def0);
  check_uninitialized(def1);

  swap(def0,opt0);
  check_uninitialized(opt0);
  check_initialized(def0);
  check_value(def0,a,z);

  // restore def0 and opt0
  swap(def0,opt0);

  swap(opt0,opt1);
  check_instance_count(count, ARG(T) );
  check_initialized(opt0);
  check_initialized(opt1);
  check_value(opt0,b,z);
  check_value(opt1,a,z);
}

//
// Test swap in a throwing case
//
template<class T>
void test_throwing_swap( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T a(16);
  T b(17);
  T x(-1);

  optional<T> opt0(a) ;
  optional<T> opt1(b) ;

  set_throw_on_assign( ARG(T) ) ;

  //
  // Case 1: Both Initialized.
  //
  bool passed = false ;
  try
  {
    // This should attempt to swap optionals and fail at swap(X&,X&).
    swap(opt0,opt1);

    passed = true ;
  }
  catch ( ... ) {}

  BOOST_CHECK(!passed);

  // optional's swap doesn't affect the initialized states of the arguments. Therefore,
  // the following must hold:
  check_initialized(opt0);
  check_initialized(opt1);
  check_value(opt0,x,a);
  check_value(opt1,b,x);


  //
  // Case 2: Only one Initialized.
  //
  reset_throw_on_assign( ARG(T) ) ;

  opt0.reset();
  opt1.reset(a);

  set_throw_on_copy( ARG(T) ) ;

  passed = false ;
  try
  {
    // This should attempt to swap optionals and fail at opt0.reset(*opt1)
    // Both opt0 and op1 are left unchanged (unswaped)
    swap(opt0,opt1);

    passed = true ;
  }
  catch ( ... ) {}

  BOOST_CHECK(!passed);

  check_uninitialized(opt0);
  check_initialized(opt1);
  check_value(opt1,a,x);

  reset_throw_on_copy( ARG(T) ) ;
}

//
// This verifies relational operators.
//
template<class T>
void test_relops( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T v0(0);
  T v1(1);
  T v2(1);

  optional<T> def0 ;
  optional<T> def1 ;
  optional<T> opt0(v0);
  optional<T> opt1(v1);
  optional<T> opt2(v2);

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

  // Compare against a value directly
  BOOST_CHECK ( opt0 == v0 ) ;
  BOOST_CHECK ( opt0 != v1 ) ;
  BOOST_CHECK ( opt1 == v2 ) ;
  BOOST_CHECK ( opt0 <  v1 ) ;
  BOOST_CHECK ( opt1 >  v0 ) ;
  BOOST_CHECK ( opt1 <= v2 ) ;
  BOOST_CHECK ( opt1 >= v0 ) ;
  BOOST_CHECK ( v0 != opt1 ) ;
  BOOST_CHECK ( v1 == opt2 ) ;
  BOOST_CHECK ( v0 <  opt1 ) ;
  BOOST_CHECK ( v1 >  opt0 ) ;
  BOOST_CHECK ( v1 <= opt2 ) ;
  BOOST_CHECK ( v1 >= opt0 ) ;
  BOOST_CHECK (   def0 != v0  ) ;
  BOOST_CHECK ( !(def0 == v0) ) ;
  BOOST_CHECK (   def0 <  v0  ) ;
  BOOST_CHECK ( !(def0 >  v0) ) ;
  BOOST_CHECK (   def0 <= v0  ) ;
  BOOST_CHECK ( !(def0 >= v0) ) ;
  BOOST_CHECK (   v0 != def0  ) ;
  BOOST_CHECK ( !(v0 == def0) ) ;
  BOOST_CHECK ( !(v0 <  def0) ) ;
  BOOST_CHECK (   v0 >  def0  ) ;
  BOOST_CHECK ( !(v0 <= def0) ) ;
  BOOST_CHECK (   v0 >= opt0  ) ;
}

template<class T>
void test_none( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  using boost::none ;

  optional<T> def0 ;
  optional<T> def1(none) ;
  optional<T> non_def( T(1234) ) ;

  BOOST_CHECK ( def0    == none ) ;
  BOOST_CHECK ( non_def != none ) ;
  BOOST_CHECK ( !def1           ) ;
  BOOST_CHECK ( !(non_def <  none) ) ;
  BOOST_CHECK (   non_def >  none  ) ;
  BOOST_CHECK ( !(non_def <= none) ) ;
  BOOST_CHECK (   non_def >= none  ) ;

  non_def = none ;
  BOOST_CHECK ( !non_def ) ;

  test_default_implicit_construction(T(1),none);
}

template<class T>
void test_arrow( T const* )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  T a(1234);

  optional<T>        oa(a) ;
  optional<T> const coa(a) ;

  BOOST_CHECK ( coa->V() == 1234 ) ;

  oa->V() = 4321 ;

  BOOST_CHECK (     a.V() = 1234 ) ;
  BOOST_CHECK ( (*oa).V() = 4321 ) ;
}

void test_with_builtin_types()
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  test_basics( ARG(double) );
  test_conditional_ctor_and_get_valur_or( ARG(double) );
  test_uninitialized_access( ARG(double) );
  test_no_throwing_swap( ARG(double) );
  test_relops( ARG(double) ) ;
  test_none( ARG(double) ) ;
}

void test_with_class_type()
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  test_basics( ARG(X) );
  test_conditional_ctor_and_get_valur_or( ARG(X) );
  test_direct_value_manip( ARG(X) );
  test_uninitialized_access( ARG(X) );
  test_throwing_direct_init( ARG(X) );
  test_throwing_val_assign_on_uninitialized( ARG(X) );
  test_throwing_val_assign_on_initialized( ARG(X) );
  test_throwing_copy_initialization( ARG(X) );
  test_throwing_assign_to_uninitialized( ARG(X) );
  test_throwing_assign_to_initialized( ARG(X) );
  test_no_throwing_swap( ARG(X) );
  test_throwing_swap( ARG(X) );
  test_relops( ARG(X) ) ;
  test_none( ARG(X) ) ;
  test_arrow( ARG(X) ) ;
  BOOST_CHECK ( X::count == 0 ) ;
}

int eat ( bool ) { return 1 ; }
int eat ( char ) { return 1 ; }
int eat ( int  ) { return 1 ; }
int eat ( void const* ) { return 1 ; }

template<class T> int eat ( T ) { return 0 ; }

//
// This verifies that operator safe_bool() behaves properly.
//
template<class T>
void test_no_implicit_conversions_impl( T const& )
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  optional<T> def ;
  BOOST_CHECK ( eat(def) == 0 ) ;
}

void test_no_implicit_conversions()
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION   );

  bool b = false ;
  char c = 0 ;
  int i = 0 ;
  void const* p = 0 ;

  test_no_implicit_conversions_impl(b);
  test_no_implicit_conversions_impl(c);
  test_no_implicit_conversions_impl(i);
  test_no_implicit_conversions_impl(p);
}

struct A {} ;
void test_conversions1()
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION );

#ifndef BOOST_OPTIONAL_NO_CONVERTING_COPY_CTOR
  char c = 20 ;
  optional<char> opt0(c);
  optional<int> opt1(opt0);
  BOOST_CHECK(*opt1 == static_cast<int>(c));
#endif

#ifndef BOOST_OPTIONAL_NO_CONVERTING_ASSIGNMENT
  float f = 21.22f ;
  double d = f ;
  optional<float> opt2(f) ;
  optional<double> opt3 ;
  opt3 = opt2 ;
  BOOST_CHECK(*opt3 == d);
#endif
}

void test_conversions2()
{
  TRACE( std::endl << BOOST_CURRENT_FUNCTION );

  char c = 20 ;
  optional<int> opt(c);
  BOOST_CHECK( get(opt) == static_cast<int>(c));

  float f = 21.22f ;
  optional<double> opt1;
  opt1 = f ;
  BOOST_CHECK(*get(&opt1) == static_cast<double>(f));
}


namespace optional_swap_test
{
  class default_ctor_exception : public std::exception {} ;
  class copy_ctor_exception : public std::exception {} ;
  class assignment_exception : public std::exception {} ;

  //
  // Base class for swap test classes.  Its assignment should not be called, when swapping
  // optional<T> objects.  (The default std::swap would do so.)
  //
  class base_class_with_forbidden_assignment
  {
  public:
    base_class_with_forbidden_assignment & operator=(const base_class_with_forbidden_assignment &)
    {
      BOOST_CHECK(!"The assignment should not be used while swapping!");
      throw assignment_exception();
    }

    virtual ~base_class_with_forbidden_assignment() {}
  };

  //
  // Class without default constructor
  //
  class class_without_default_ctor : public base_class_with_forbidden_assignment
  {
  public:
    char data;
    explicit class_without_default_ctor(char arg) : data(arg) {}
  };

  //
  // Class whose default constructor should not be used by optional::swap!
  //
  class class_whose_default_ctor_should_not_be_used : public base_class_with_forbidden_assignment
  {
  public:
    char data;
    explicit class_whose_default_ctor_should_not_be_used(char arg) : data(arg) {}

    class_whose_default_ctor_should_not_be_used()
    {
      BOOST_CHECK(!"This default constructor should not be used while swapping!");
      throw default_ctor_exception();
    }
  };

  //
  // Class whose default constructor should be used by optional::swap.
  // Its copy constructor should be avoided!
  //
  class class_whose_default_ctor_should_be_used : public base_class_with_forbidden_assignment
  {
  public:
    char data;
    explicit class_whose_default_ctor_should_be_used(char arg) : data(arg) { }

    class_whose_default_ctor_should_be_used() : data('\0') { }

    class_whose_default_ctor_should_be_used(const class_whose_default_ctor_should_be_used &)
    {
      BOOST_CHECK(!"This copy constructor should not be used while swapping!");
      throw copy_ctor_exception();
    }
  };

  //
  // Class template whose default constructor should be used by optional::swap.
  // Its copy constructor should be avoided!
  //
  template <class T>
  class template_whose_default_ctor_should_be_used : public base_class_with_forbidden_assignment
  {
  public:
    T data;
    explicit template_whose_default_ctor_should_be_used(T arg) : data(arg) { }

    template_whose_default_ctor_should_be_used() : data('\0') { }

    template_whose_default_ctor_should_be_used(const template_whose_default_ctor_should_be_used &)
    {
      BOOST_CHECK(!"This copy constructor should not be used while swapping!");
      throw copy_ctor_exception();
    }
  };

  //
  // Class whose explicit constructor should be used by optional::swap.
  // Its other constructors should be avoided!
  //
  class class_whose_explicit_ctor_should_be_used : public base_class_with_forbidden_assignment
  {
  public:
    char data;
    explicit class_whose_explicit_ctor_should_be_used(char arg) : data(arg) { }

    class_whose_explicit_ctor_should_be_used()
    {
      BOOST_CHECK(!"This default constructor should not be used while swapping!");
      throw default_ctor_exception();
    }

    class_whose_explicit_ctor_should_be_used(const class_whose_explicit_ctor_should_be_used &)
    {
      BOOST_CHECK(!"This copy constructor should not be used while swapping!");
      throw copy_ctor_exception();
    }
  };

  void swap(class_whose_default_ctor_should_not_be_used & lhs, class_whose_default_ctor_should_not_be_used & rhs)
  {
    std::swap(lhs.data, rhs.data);
  }

  void swap(class_whose_default_ctor_should_be_used & lhs, class_whose_default_ctor_should_be_used & rhs)
  {
    std::swap(lhs.data, rhs.data);
  }

  void swap(class_without_default_ctor & lhs, class_without_default_ctor & rhs)
  {
    std::swap(lhs.data, rhs.data);
  }

  void swap(class_whose_explicit_ctor_should_be_used & lhs, class_whose_explicit_ctor_should_be_used & rhs)
  {
    std::swap(lhs.data, rhs.data);
  }

  template <class T>
  void swap(template_whose_default_ctor_should_be_used<T> & lhs, template_whose_default_ctor_should_be_used<T> & rhs)
  {
    std::swap(lhs.data, rhs.data);
  }

  //
  // optional<T>::swap should be customized when neither the copy constructor
  // nor the default constructor of T are supposed to be used when swapping, e.g.,
  // for the following type T = class_whose_explicit_ctor_should_be_used.
  //
  void swap(boost::optional<class_whose_explicit_ctor_should_be_used> & x, boost::optional<class_whose_explicit_ctor_should_be_used> & y)
  {
    bool hasX = x;
    bool hasY = y;

    if ( !hasX && !hasY )
     return;

    if( !hasX )
       x = boost::in_place('\0');
    else if ( !hasY )
       y = boost::in_place('\0');

    optional_swap_test::swap(*x,*y);

     if( !hasX )
         y = boost::none ;
     else if( !hasY )
         x = boost::none ;
  }


} // End of namespace optional_swap_test.


namespace boost {

//
// Compile time tweaking on whether or not swap should use the default constructor:
//

template <> struct optional_swap_should_use_default_constructor<
  optional_swap_test::class_whose_default_ctor_should_be_used> : mpl::true_ {} ;

template <> struct optional_swap_should_use_default_constructor<
  optional_swap_test::class_whose_default_ctor_should_not_be_used> : mpl::false_ {} ;

template <class T> struct optional_swap_should_use_default_constructor<
  optional_swap_test::template_whose_default_ctor_should_be_used<T> > : mpl::true_ {} ;


//
// Specialization of boost::swap:
//
template <>
void swap(optional<optional_swap_test::class_whose_explicit_ctor_should_be_used> & x, optional<optional_swap_test::class_whose_explicit_ctor_should_be_used> & y)
{
  optional_swap_test::swap(x, y);
}

} // namespace boost


namespace std {

//
// Specializations of std::swap:
//

template <>
void swap(optional_swap_test::class_whose_default_ctor_should_be_used & x, optional_swap_test::class_whose_default_ctor_should_be_used & y)
{
  optional_swap_test::swap(x, y);
}

template <>
void swap(optional_swap_test::class_whose_default_ctor_should_not_be_used & x, optional_swap_test::class_whose_default_ctor_should_not_be_used & y)
{
  optional_swap_test::swap(x, y);
}

template <>
void swap(optional_swap_test::class_without_default_ctor & x, optional_swap_test::class_without_default_ctor & y)
{
  optional_swap_test::swap(x, y);
}

template <>
void swap(optional_swap_test::class_whose_explicit_ctor_should_be_used & x, optional_swap_test::class_whose_explicit_ctor_should_be_used & y)
{
  optional_swap_test::swap(x, y);
}

} // namespace std


//
// Tests whether the swap function works properly for optional<T>.
// Assumes that T has one data member, of type char.
// Returns true iff the test is passed.
//
template <class T>
bool test_swap_function( T const* )
{
  const boost::unit_test::counter_t counter_before_test = boost::minimal_test::errors_counter();
  try
  {
    optional<T> obj1;
    optional<T> obj2('a');

    // Self-swap should not have any effect.
    swap(obj1, obj1);
    swap(obj2, obj2);
    BOOST_CHECK(!obj1);
    BOOST_CHECK(!!obj2 && obj2->data == 'a');

    // Call non-member swap.
    swap(obj1, obj2);

    // Test if obj1 and obj2 are really swapped.
    BOOST_CHECK(!!obj1 && obj1->data == 'a');
    BOOST_CHECK(!obj2);

    // Call non-member swap one more time.
    swap(obj1, obj2);

    // Test if obj1 and obj2 are swapped back.
    BOOST_CHECK(!obj1);
    BOOST_CHECK(!!obj2 && obj2->data == 'a');
  }
  catch(const std::exception &)
  {
    // The swap function should not throw, for our test cases.
    return false ;
  }
  return boost::minimal_test::errors_counter() == counter_before_test ;
}

//
// Tests whether the optional<T>::swap member function works properly.
// Assumes that T has one data member, of type char.
// Returns true iff the test is passed.
//
template <class T>
bool test_swap_member_function( T const* )
{
  const boost::unit_test::counter_t counter_before_test = boost::minimal_test::errors_counter();
  try
  {
    optional<T> obj1;
    optional<T> obj2('a');

    // Self-swap should not have any effect.
    obj1.swap(obj1);
    obj2.swap(obj2);
    BOOST_CHECK(!obj1);
    BOOST_CHECK(!!obj2 && obj2->data == 'a');

    // Call member swap.
    obj1.swap(obj2);

    // Test if obj1 and obj2 are really swapped.
    BOOST_CHECK(!!obj1 && obj1->data == 'a');
    BOOST_CHECK(!obj2);

    // Call member swap one more time.
    obj1.swap(obj2);

    // Test if obj1 and obj2 are swapped back.
    BOOST_CHECK(!obj1);
    BOOST_CHECK(!!obj2 && obj2->data == 'a');
  }
  catch(const std::exception &)
  {
    // The optional<T>::swap member function should not throw, for our test cases.
    return false ;
  }
  return boost::minimal_test::errors_counter() == counter_before_test ;
}


//
// Tests compile time tweaking of swap, by means of
// optional_swap_should_use_default_constructor.
//
void test_swap_tweaking()
{
  BOOST_CHECK( test_swap_function( ARG(optional_swap_test::class_without_default_ctor) ) );
  BOOST_CHECK( test_swap_function( ARG(optional_swap_test::class_whose_default_ctor_should_be_used) ) );
  BOOST_CHECK( test_swap_function( ARG(optional_swap_test::class_whose_default_ctor_should_not_be_used) ) );
  BOOST_CHECK( test_swap_function( ARG(optional_swap_test::class_whose_explicit_ctor_should_be_used) ) );
  BOOST_CHECK( test_swap_function( ARG(optional_swap_test::template_whose_default_ctor_should_be_used<char>) ) );
  BOOST_CHECK( test_swap_member_function( ARG(optional_swap_test::class_without_default_ctor) ) );
  BOOST_CHECK( test_swap_member_function( ARG(optional_swap_test::class_whose_default_ctor_should_be_used) ) );
  BOOST_CHECK( test_swap_member_function( ARG(optional_swap_test::class_whose_default_ctor_should_not_be_used) ) );
  BOOST_CHECK( test_swap_member_function( ARG(optional_swap_test::class_whose_explicit_ctor_should_be_used) ) );
  BOOST_CHECK( test_swap_member_function( ARG(optional_swap_test::template_whose_default_ctor_should_be_used<char>) ) );
}

// Test for support for classes with overridden operator&
class CustomAddressOfClass
{
    int n;

public:
    CustomAddressOfClass() : n(0) {}
    CustomAddressOfClass(CustomAddressOfClass const& that) : n(that.n) {}
    explicit CustomAddressOfClass(int m) : n(m) {}
    int* operator& () { return &n; }
    bool operator== (CustomAddressOfClass const& that) const { return n == that.n; }
};

void test_custom_addressof_operator()
{
    boost::optional< CustomAddressOfClass > o1(CustomAddressOfClass(10));
    BOOST_CHECK(!!o1);
    BOOST_CHECK(o1.get() == CustomAddressOfClass(10));

    o1 = CustomAddressOfClass(20);
    BOOST_CHECK(!!o1);
    BOOST_CHECK(o1.get() == CustomAddressOfClass(20));

    o1 = boost::none;
    BOOST_CHECK(!o1);
}

int test_main( int, char* [] )
{
  try
  {
    test_with_class_type();
    test_with_builtin_types();
    test_no_implicit_conversions();
    test_conversions1();
    test_conversions2();
    test_swap_tweaking();
    test_custom_addressof_operator();
  }
  catch ( ... )
  {
    BOOST_ERROR("Unexpected Exception caught!");
  }

  return 0;
}


