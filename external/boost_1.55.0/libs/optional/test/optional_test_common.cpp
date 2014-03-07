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
#ifdef ENABLE_TRACE
#define TRACE(msg) std::cout << msg << std::endl ;
#else
#define TRACE(msg)
#endif

namespace boost {

void assertion_failed (char const * expr, char const * func, char const * file, long )
{
  using std::string ;
  string msg =  string("Boost assertion failure for \"")
               + string(expr)
               + string("\" at file \"")
               + string(file)
               + string("\" function \"")
               + string(func)
               + string("\"") ;

  TRACE(msg);

  throw std::logic_error(msg);
}

}

using boost::optional ;

template<class T> inline void unused_variable ( T ) {}

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
using boost::swap ;
using boost::get ;
using boost::get_pointer ;
#endif

// MSVC6.0 does not support comparisons of optional against a literal null pointer value (0)
// via the safe_bool operator.
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1300) ) // 1300 == VC++ 7.1
#define BOOST_OPTIONAL_NO_NULL_COMPARE
#endif

#define ARG(T) (static_cast< T const* >(0))

//
// Helper class used to verify the lifetime managment of the values held by optional
//
class X
{
  public :

    X ( int av ) : v(av)
    {
      ++ count ;

      TRACE ( "X::X(" << av << "). this=" << this ) ;
    }

    X ( X const& rhs ) : v(rhs.v)
    {
       pending_copy = false ;

       TRACE ( "X::X( X const& rhs). this=" << this << " rhs.v=" << rhs.v ) ;

       if ( throw_on_copy )
       {
         TRACE ( "throwing exception in X's copy ctor" ) ;
         throw 0 ;
       }

       ++ count ;
    }

    ~X()
    {
      pending_dtor = false ;

      -- count ;

      TRACE ( "X::~X(). v=" << v << "  this=" << this );
    }

    X& operator= ( X const& rhs )
      {
        pending_assign = false ;

        if ( throw_on_assign )
        {
          TRACE ( "throwing exception in X's assignment" ) ;

          v = -1 ;

          throw 0 ;
        }
        else
        {
          v = rhs.v ;

          TRACE ( "X::operator =( X const& rhs). this=" << this << " rhs.v=" << rhs.v ) ;
        }
        return *this ;
      }

    friend bool operator == ( X const& a, X const& b )
      { return a.v == b.v ; }

    friend bool operator != ( X const& a, X const& b )
      { return a.v != b.v ; }

    friend bool operator < ( X const& a, X const& b )
      { return a.v < b.v ; }

    int  V() const { return v ; }
    int& V()       { return v ; }

    static int  count ;
    static bool pending_copy   ;
    static bool pending_dtor   ;
    static bool pending_assign ;
    static bool throw_on_copy ;
    static bool throw_on_assign ;

  private :

    int  v ;

  private :

    X() ;
} ;


int  X::count           = 0 ;
bool X::pending_copy    = false ;
bool X::pending_dtor    = false ;
bool X::pending_assign  = false ;
bool X::throw_on_copy   = false ;
bool X::throw_on_assign = false ;

inline void set_pending_copy           ( X const* x ) { X::pending_copy  = true  ; }
inline void set_pending_dtor           ( X const* x ) { X::pending_dtor  = true  ; }
inline void set_pending_assign         ( X const* x ) { X::pending_assign = true  ; }
inline void set_throw_on_copy          ( X const* x ) { X::throw_on_copy = true  ; }
inline void set_throw_on_assign        ( X const* x ) { X::throw_on_assign = true  ; }
inline void reset_throw_on_copy        ( X const* x ) { X::throw_on_copy = false ; }
inline void reset_throw_on_assign      ( X const* x ) { X::throw_on_assign = false ; }
inline void check_is_pending_copy      ( X const* x ) { BOOST_CHECK( X::pending_copy ) ; }
inline void check_is_pending_dtor      ( X const* x ) { BOOST_CHECK( X::pending_dtor ) ; }
inline void check_is_pending_assign    ( X const* x ) { BOOST_CHECK( X::pending_assign ) ; }
inline void check_is_not_pending_copy  ( X const* x ) { BOOST_CHECK( !X::pending_copy ) ; }
inline void check_is_not_pending_dtor  ( X const* x ) { BOOST_CHECK( !X::pending_dtor ) ; }
inline void check_is_not_pending_assign( X const* x ) { BOOST_CHECK( !X::pending_assign ) ; }
inline void check_instance_count       ( int c, X const* x ) { BOOST_CHECK( X::count == c ) ; }
inline int  get_instance_count         ( X const* x ) { return X::count ; }

inline void set_pending_copy           (...) {}
inline void set_pending_dtor           (...) {}
inline void set_pending_assign         (...) {}
inline void set_throw_on_copy          (...) {}
inline void set_throw_on_assign        (...) {}
inline void reset_throw_on_copy        (...) {}
inline void reset_throw_on_assign      (...) {}
inline void check_is_pending_copy      (...) {}
inline void check_is_pending_dtor      (...) {}
inline void check_is_pending_assign    (...) {}
inline void check_is_not_pending_copy  (...) {}
inline void check_is_not_pending_dtor  (...) {}
inline void check_is_not_pending_assign(...) {}
inline void check_instance_count       (...) {}
inline int  get_instance_count         (...) { return 0 ; }


template<class T>
inline void check_uninitialized_const ( optional<T> const& opt )
{
#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt == 0 ) ;
#endif
  BOOST_CHECK( !opt ) ;
  BOOST_CHECK( !get_pointer(opt) ) ;
  BOOST_CHECK( !opt.get_ptr() ) ;
}
template<class T>
inline void check_uninitialized ( optional<T>& opt )
{
#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt == 0 ) ;
#endif
  BOOST_CHECK( !opt ) ;
  BOOST_CHECK( !get_pointer(opt) ) ;
  BOOST_CHECK( !opt.get_ptr() ) ;

  check_uninitialized_const(opt);
}

template<class T>
inline void check_initialized_const ( optional<T> const& opt )
{
  BOOST_CHECK( opt ) ;

#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt != 0 ) ;
#endif

  BOOST_CHECK ( !!opt ) ;
  BOOST_CHECK ( get_pointer(opt) ) ;
  BOOST_CHECK ( opt.get_ptr() ) ;
}

template<class T>
inline void check_initialized ( optional<T>& opt )
{
  BOOST_CHECK( opt ) ;

#ifndef BOOST_OPTIONAL_NO_NULL_COMPARE
  BOOST_CHECK( opt != 0 ) ;
#endif

  BOOST_CHECK ( !!opt ) ;
  BOOST_CHECK ( get_pointer(opt) ) ;
  BOOST_CHECK ( opt.get_ptr() ) ;

  check_initialized_const(opt);
}

template<class T>
inline void check_value_const ( optional<T> const& opt, T const& v, T const& z )
{
  BOOST_CHECK( *opt == v ) ;
  BOOST_CHECK( *opt != z ) ;
  BOOST_CHECK( opt.get() == v ) ;
  BOOST_CHECK( opt.get() != z ) ;
  BOOST_CHECK( (*(opt.operator->()) == v) ) ;
  BOOST_CHECK( *get_pointer(opt) == v ) ;
}

template<class T>
inline void check_value ( optional<T>& opt, T const& v, T const& z )
{
#if BOOST_WORKAROUND(BOOST_MSVC, <= 1200) // 1200 == VC++ 6.0
  // For some reason, VC6.0 is creating a temporary while evaluating (*opt == v),
  // so we need to turn throw on copy off first.
  reset_throw_on_copy( ARG(T) ) ;
#endif

  BOOST_CHECK( *opt == v ) ;
  BOOST_CHECK( *opt != z ) ;
  BOOST_CHECK( opt.get() == v ) ;
  BOOST_CHECK( opt.get() != z ) ;
  BOOST_CHECK( (*(opt.operator->()) == v) ) ;
  BOOST_CHECK( *get_pointer(opt) == v ) ;

  check_value_const(opt,v,z);
}


