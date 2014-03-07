
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <cstdio>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/move/move.hpp>
#include <boost/range.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/utility.hpp>

#include <boost/coroutine/all.hpp>

namespace coro = boost::coroutines;

int value1 = 0;
std::string value2 = "";
bool value3 = false;
double value4 = .0;
int * value5 = 0;
int& value6 = value1;
int& value7 = value1;
int value8 = 0;
int value9 = 0;

#ifdef BOOST_COROUTINES_UNIDIRECT
struct X : private boost::noncopyable
{
    X() { value1 = 7; }
    ~X() { value1 = 0; }
};

class copyable
{
public:
    bool    state;

    copyable() :
        state( false)
    {}

    copyable( int) :
        state( true)
    {}

    void operator()( coro::coroutine< int >::push_type &)
    { value3 = state; }
};

class moveable
{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE( moveable);

public:
    bool    state;

    moveable() :
        state( false)
    {}

    moveable( int) :
        state( true)
    {}

    moveable( BOOST_RV_REF( moveable) other) :
        state( false)
    { std::swap( state, other.state); }

    moveable & operator=( BOOST_RV_REF( moveable) other)
    {
        if ( this == & other) return * this;
        moveable tmp( boost::move( other) );
        std::swap( state, tmp.state);
        return * this;
    }

    void operator()( coro::coroutine< int >::push_type &)
    { value3 = state; }
};

struct my_exception {};

void f1( coro::coroutine< void >::push_type & c)
{ c(); }

void f2( coro::coroutine< void >::push_type &)
{ ++value1; }

void f3( coro::coroutine< void >::push_type & c)
{
    ++value1;
    c();
    ++value1;
}

void f4( coro::coroutine< int >::push_type & c)
{
    c( 3);
    c( 7);
}

void f5( coro::coroutine< std::string >::push_type & c)
{
    std::string res("abc");
    c( res);
    res = "xyz";
    c( res);
}

void f6( coro::coroutine< int >::pull_type & c)
{ value1 = c.get(); }

void f7( coro::coroutine< std::string >::pull_type & c)
{ value2 = c.get(); }

void f8( coro::coroutine< boost::tuple< double, double > >::pull_type & c)
{
    double x = 0, y = 0;
    boost::tie( x, y) = c.get();
    value4 = x + y;
    c();
    boost::tie( x, y) = c.get();
    value4 = x + y;
}

void f9( coro::coroutine< int * >::pull_type & c)
{ value5 = c.get(); }

void f91( coro::coroutine< int const* >::pull_type & c)
{ value5 = const_cast< int * >( c.get() ); }

void f10( coro::coroutine< int & >::pull_type & c)
{
    int const& i = c.get();
    value5 = const_cast< int * >( & i);
}

void f101( coro::coroutine< int const& >::pull_type & c)
{
    int const& i = c.get();
    value5 = const_cast< int * >( & i);
}

void f11( coro::coroutine< boost::tuple< int, int > >::pull_type & c)
{
    boost::tie( value8, value9) = c.get();
}

void f12( coro::coroutine< void >::pull_type & c)
{
    X x_;
    c();
    c();
}

template< typename E >
void f14( coro::coroutine< void >::pull_type &, E const& e)
{ throw e; }

void f16( coro::coroutine< int >::push_type & c)
{
    c( 1);
    c( 2);
    c( 3);
    c( 4);
    c( 5);
}

void f17( coro::coroutine< int >::pull_type & c, std::vector< int > & vec)
{
    int x = c.get();
    while ( 5 > x)
    {
        vec.push_back( x);
        x = c().get();
    }
}

void f19( coro::coroutine< const int* >::push_type & c, std::vector< const int * > & vec)
{
    BOOST_FOREACH( const int * ptr, vec)
    { c( ptr); }
}

void f20( coro::coroutine< int >::push_type &)
{}

void test_move()
{
    {
        coro::coroutine< void >::pull_type coro1;
        coro::coroutine< void >::pull_type coro2( f1);
        BOOST_CHECK( ! coro1);
        BOOST_CHECK( coro1.empty() );
        BOOST_CHECK( coro2);
        BOOST_CHECK( ! coro2.empty() );
        coro1 = boost::move( coro2);
        BOOST_CHECK( coro1);
        BOOST_CHECK( ! coro1.empty() );
        BOOST_CHECK( ! coro2);
        BOOST_CHECK( coro2.empty() );
    }

    {
        value3 = false;
        copyable cp( 3);
        BOOST_CHECK( cp.state);
        BOOST_CHECK( ! value3);
        coro::coroutine< int >::pull_type coro( cp);
        BOOST_CHECK( cp.state);
        BOOST_CHECK( value3);
    }

    {
        value3 = false;
        moveable mv( 7);
        BOOST_CHECK( mv.state);
        BOOST_CHECK( ! value3);
        coro::coroutine< int >::pull_type coro( boost::move( mv) );
        BOOST_CHECK( ! mv.state);
        BOOST_CHECK( value3);
    }
}

void test_complete()
{
    value1 = 0;

    coro::coroutine< void >::pull_type coro( f2);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( ( int)1, value1);
}

void test_jump()
{
    value1 = 0;

    coro::coroutine< void >::pull_type coro( f3);
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( ( int)1, value1);
    coro();
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( ( int)2, value1);
}

void test_result_int()
{
    coro::coroutine< int >::pull_type coro( f4);
    BOOST_CHECK( coro);
    int result = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( 3, result);
    result = coro().get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( 7, result);
    coro();
    BOOST_CHECK( ! coro);
}

void test_result_string()
{
    coro::coroutine< std::string >::pull_type coro( f5);
    BOOST_CHECK( coro);
    std::string result = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( std::string("abc"), result);
    result = coro().get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( std::string("xyz"), result);
    coro();
    BOOST_CHECK( ! coro);
}

void test_arg_int()
{
    value1 = 0;

    coro::coroutine< int >::push_type coro( f6);
    BOOST_CHECK( coro);
    coro( 3);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( 3, value1);
}

void test_arg_string()
{
    value2 = "";

    coro::coroutine< std::string >::push_type coro( f7);
    BOOST_CHECK( coro);
    coro( std::string("abc") );
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( std::string("abc"), value2);
}

void test_fp()
{
    value4 = 0;

    coro::coroutine< boost::tuple< double, double > >::push_type coro( f8);
    BOOST_CHECK( coro);
    coro( boost::make_tuple( 7.35, 3.14) );
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( ( double) 10.49, value4);

    value4 = 0;
    coro( boost::make_tuple( 1.15, 3.14) );
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( ( double) 4.29, value4);
}

void test_ptr()
{
    value5 = 0;

    int a = 3;
    coro::coroutine< int * >::push_type coro( f9);
    BOOST_CHECK( coro);
    coro( & a);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( & a, value5);
}

void test_const_ptr()
{
    value5 = 0;

    int a = 3;
    coro::coroutine< int const* >::push_type coro( f91);
    BOOST_CHECK( coro);
    coro( & a);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( & a, value5);
}

void test_ref()
{
    value5 = 0;

    int a = 3;
    coro::coroutine< int & >::push_type coro( f10);
    BOOST_CHECK( coro);
    coro( a);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( & a, value5);
}

void test_const_ref()
{
    value5 = 0;

    int a = 3;
    coro::coroutine< int const& >::push_type coro( f101);
    BOOST_CHECK( coro);
    coro( a);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( & a, value5);
}

void test_tuple()
{
    value8 = 0;
    value9 = 0;

    int a = 3, b = 7;
    boost::tuple< int, int > tpl( a, b);
    BOOST_CHECK_EQUAL( a, tpl.get< 0 >() );
    BOOST_CHECK_EQUAL( b, tpl.get< 1 >() );
    coro::coroutine< boost::tuple< int, int > >::push_type coro( f11);
    BOOST_CHECK( coro);
    coro( tpl);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( a, value8);
    BOOST_CHECK_EQUAL( b, value9);
}

void test_unwind()
{
    value1 = 0;
    {
        coro::coroutine< void >::push_type coro( f12);
        BOOST_CHECK( coro);
        BOOST_CHECK_EQUAL( ( int) 0, value1);
        coro();
        BOOST_CHECK( coro);
        BOOST_CHECK_EQUAL( ( int) 7, value1);
        coro();
        BOOST_CHECK_EQUAL( ( int) 7, value1);
    }
    BOOST_CHECK_EQUAL( ( int) 0, value1);
}

void test_no_unwind()
{
    value1 = 0;
    {
        coro::coroutine< void >::push_type coro(
            f12,
            coro::attributes(
                coro::stack_allocator::default_stacksize(),
                coro::no_stack_unwind) );
        BOOST_CHECK( coro);
        BOOST_CHECK_EQUAL( ( int) 0, value1);
        coro();
        BOOST_CHECK( coro);
        BOOST_CHECK_EQUAL( ( int) 7, value1);
        coro();
        BOOST_CHECK_EQUAL( ( int) 7, value1);
    }
    BOOST_CHECK_EQUAL( ( int) 7, value1);
}

void test_exceptions()
{
    bool thrown = false;
    std::runtime_error ex("abc");
    try
    {
        coro::coroutine< void >::push_type coro( boost::bind( f14< std::runtime_error >, _1, ex) );
        BOOST_CHECK( coro);
        coro();
        BOOST_CHECK( ! coro);
        BOOST_CHECK( false);
    }
    catch ( std::runtime_error const&)
    { thrown = true; }
    catch ( std::exception const&)
    {}
    catch (...)
    {}
    BOOST_CHECK( thrown);
}

void test_output_iterator()
{
    {
        std::vector< int > vec;
        coro::coroutine< int >::pull_type coro( f16);
        BOOST_FOREACH( int i, coro)
        { vec.push_back( i); }
        BOOST_CHECK_EQUAL( ( std::size_t)5, vec.size() );
        BOOST_CHECK_EQUAL( ( int)1, vec[0] );
        BOOST_CHECK_EQUAL( ( int)2, vec[1] );
        BOOST_CHECK_EQUAL( ( int)3, vec[2] );
        BOOST_CHECK_EQUAL( ( int)4, vec[3] );
        BOOST_CHECK_EQUAL( ( int)5, vec[4] );
    }
    {
        std::vector< int > vec;
        coro::coroutine< int >::pull_type coro( f16);
        coro::coroutine< int >::pull_type::iterator e = boost::end( coro);
        for (
            coro::coroutine< int >::pull_type::iterator i = boost::begin( coro);
            i != e; ++i)
        { vec.push_back( * i); }
        BOOST_CHECK_EQUAL( ( std::size_t)5, vec.size() );
        BOOST_CHECK_EQUAL( ( int)1, vec[0] );
        BOOST_CHECK_EQUAL( ( int)2, vec[1] );
        BOOST_CHECK_EQUAL( ( int)3, vec[2] );
        BOOST_CHECK_EQUAL( ( int)4, vec[3] );
        BOOST_CHECK_EQUAL( ( int)5, vec[4] );
    }
    {
        int i1 = 1, i2 = 2, i3 = 3;
        std::vector< const int* > vec_in;
        vec_in.push_back( & i1);
        vec_in.push_back( & i2);
        vec_in.push_back( & i3);
        std::vector< const int* > vec_out;
        coro::coroutine< const int* >::pull_type coro( boost::bind( f19, _1, boost::ref( vec_in) ) );
        coro::coroutine< const int* >::pull_type::const_iterator e = boost::const_end( coro);
        for (
            coro::coroutine< const int* >::pull_type::const_iterator i = boost::const_begin( coro);
            i != e; ++i)
        { vec_out.push_back( * i); }
        BOOST_CHECK_EQUAL( ( std::size_t)3, vec_out.size() );
        BOOST_CHECK_EQUAL( & i1, vec_out[0] );
        BOOST_CHECK_EQUAL( & i2, vec_out[1] );
        BOOST_CHECK_EQUAL( & i3, vec_out[2] );
    }
}

void test_input_iterator()
{
    int counter = 0;
    std::vector< int > vec;
    coro::coroutine< int >::push_type coro(
        boost::bind( f17, _1, boost::ref( vec) ) );
    coro::coroutine< int >::push_type::iterator e( boost::end( coro) );
    for ( coro::coroutine< int >::push_type::iterator i( boost::begin( coro) );
          i != e; ++i)
    {
        i = ++counter;
    }
    BOOST_CHECK_EQUAL( ( std::size_t)4, vec.size() );
    BOOST_CHECK_EQUAL( ( int)1, vec[0] );
    BOOST_CHECK_EQUAL( ( int)2, vec[1] );
    BOOST_CHECK_EQUAL( ( int)3, vec[2] );
    BOOST_CHECK_EQUAL( ( int)4, vec[3] );
}

void test_invalid_result()
{
    bool catched = false;
    coro::coroutine< int >::pull_type coro( f20);
    BOOST_CHECK( ! coro);
    try
    {
        int i = coro.get();
        (void)i;
    }
    catch ( coro::invalid_result const&)
    {
        catched = true; 
    }
    BOOST_CHECK( catched);
}
#else
typedef coro::coroutine< void() > coro_void_void;
typedef coro::coroutine< int() > coro_int_void;
typedef coro::coroutine< std::string() > coro_string_void;
typedef coro::coroutine< void(int) > coro_void_int;
typedef coro::coroutine< void(std::string const&) > coro_void_string;
typedef coro::coroutine< double(double,double) > coro_double;
typedef coro::coroutine< int(int,int) > coro_int;
typedef coro::coroutine< int(int) > coro_int_int;
typedef coro::coroutine< int*(int*) > coro_ptr;
typedef coro::coroutine< int const*(int const*) > coro_const_ptr;
typedef coro::coroutine< int&(int&) > coro_ref;
typedef coro::coroutine< int const&(int const&) > coro_const_ref;
typedef coro::coroutine< boost::tuple<int&,int&>(int&,int&) > coro_tuple;
typedef coro::coroutine< const int *() > coro_const_int_ptr_void;

struct X : private boost::noncopyable
{
    X() { value1 = 7; }
    ~X() { value1 = 0; }
};

class copyable
{
public:
    bool    state;

    copyable() :
        state( false)
    {}

    copyable( int) :
        state( true)
    {}

    void operator()( coro_int_void::caller_type &)
    { value3 = state; }
};

class moveable
{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE( moveable);

public:
    bool    state;

    moveable() :
        state( false)
    {}

    moveable( int) :
        state( true)
    {}

    moveable( BOOST_RV_REF( moveable) other) :
        state( false)
    { std::swap( state, other.state); }

    moveable & operator=( BOOST_RV_REF( moveable) other)
    {
        if ( this == & other) return * this;
        moveable tmp( boost::move( other) );
        std::swap( state, tmp.state);
        return * this;
    }

    void operator()( coro_int_void::caller_type &)
    { value3 = state; }
};

struct my_exception {};

void f1( coro_void_void::caller_type & s)
{ s(); }

void f2( coro_void_void::caller_type &)
{ ++value1; }

void f3( coro_void_void::caller_type & self)
{
    ++value1;
    self();
    ++value1;
}

void f4( coro_int_void::caller_type & self)
{
    self( 3);
    self( 7);
}

void f5( coro_string_void::caller_type & self)
{
    std::string res("abc");
    self( res);
    res = "xyz";
    self( res);
}

void f6( coro_void_int::caller_type & self)
{ value1 = self.get(); }

void f7( coro_void_string::caller_type & self)
{ value2 = self.get(); }

void f8( coro_double::caller_type & self)
{
    double x = 0, y = 0;
    boost::tie( x, y) = self.get();
    self( x + y);
    boost::tie( x, y) = self.get();
    self( x + y);
}

void f9( coro_ptr::caller_type & self)
{ self( self.get() ); }

void f91( coro_const_ptr::caller_type & self)
{ self( self.get() ); }

void f10( coro_ref::caller_type & self)
{ self( self.get() ); }

void f101( coro_const_ref::caller_type & self)
{ self( self.get() ); }

void f11( coro_tuple::caller_type & self)
{
    boost::tuple<int&,int&> tpl( self.get().get< 0 >(), self.get().get< 1 >() );
    self( tpl);
    //self( 7, 11); //TODO: does not work
}

void f12( coro_int::caller_type & self)
{
    X x_;
    int x, y;
    boost::tie( x, y) = self.get();
    self( x +y);
    boost::tie( x, y) = self.get();
    self( x +y);
}

template< typename E >
void f14( coro_void_void::caller_type & self, E const& e)
{ throw e; }

void f16( coro_int_void::caller_type & self)
{
    self( 1);
    self( 2);
    self( 3);
    self( 4);
    self( 5);
}

void f17( coro_void_int::caller_type & self, std::vector< int > & vec)
{
    int x = self.get();
    while ( 5 > x)
    {
        vec.push_back( x);
        x = self().get();
    }
}

void f18( coro_int_int::caller_type & self)
{
    if ( self.has_result() )
    {
        int x = self.get();
        self( x + 1);
    }
    else
    {
        self( -1);
    }
}

void f19( coro_const_int_ptr_void::caller_type & self, std::vector< const int * > & vec)
{
    BOOST_FOREACH( const int * ptr, vec)
    { self( ptr); }
}

void test_move()
{
    {
        coro_void_void coro1;
        coro_void_void coro2( f1);
        BOOST_CHECK( ! coro1);
        BOOST_CHECK( coro1.empty() );
        BOOST_CHECK( coro2);
        BOOST_CHECK( ! coro2.empty() );
        coro1 = boost::move( coro2);
        BOOST_CHECK( coro1);
        BOOST_CHECK( ! coro1.empty() );
        BOOST_CHECK( ! coro2);
        BOOST_CHECK( coro2.empty() );
    }

    {
        value3 = false;
        copyable cp( 3);
        BOOST_CHECK( cp.state);
        BOOST_CHECK( ! value3);
        coro_int_void coro( cp);
        BOOST_CHECK( cp.state);
        BOOST_CHECK( value3);
    }

    {
        value3 = false;
        moveable mv( 7);
        BOOST_CHECK( mv.state);
        BOOST_CHECK( ! value3);
        coro_int_void coro( boost::move( mv) );
        BOOST_CHECK( ! mv.state);
        BOOST_CHECK( value3);
    }
}

void test_complete()
{
    value1 = 0;

    coro_void_void coro( f2);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( ( int)1, value1);
}

void test_jump()
{
    value1 = 0;

    coro_void_void coro( f3);
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( ( int)1, value1);
    coro();
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( ( int)2, value1);
}

void test_result_int()
{
    coro_int_void coro( f4);
    BOOST_CHECK( coro);
    int result = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( 3, result);
    result = coro().get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( 7, result);
    coro();
    BOOST_CHECK( ! coro);
}

void test_result_string()
{
    coro_string_void coro( f5);
    BOOST_CHECK( coro);
    std::string result = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( std::string("abc"), result);
    result = coro().get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( std::string("xyz"), result);
    coro();
    BOOST_CHECK( ! coro);
}

void test_arg_int()
{
    value1 = 0;

    coro_void_int coro( f6, 3);
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( 3, value1);
}

void test_arg_string()
{
    value2 = "";

    coro_void_string coro( f7, std::string("abc") );
    BOOST_CHECK( ! coro);
    BOOST_CHECK_EQUAL( std::string("abc"), value2);
}

void test_fp()
{
    coro_double coro( f8, coro_double::arguments( 7.35, 3.14) );
    BOOST_CHECK( coro);
    double res = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( ( double) 10.49, res);
    res = coro( 1.15, 3.14).get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( ( double) 4.29, res);
    coro( 1.15, 3.14);
    BOOST_CHECK( ! coro);
}

void test_ptr()
{
    int a = 3;
    coro_ptr coro( f9, & a);
    BOOST_CHECK( coro);
    int * res = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( & a, res);
    coro( & a);
    BOOST_CHECK( ! coro);
}

void test_const_ptr()
{
    int a = 3;
    coro_const_ptr coro( f91, & a);
    BOOST_CHECK( coro);
    int const* res = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( & a, res);
    coro( & a);
    BOOST_CHECK( ! coro);
}

void test_ref()
{
    int a = 3;
    coro_ref coro( f10, a);
    BOOST_CHECK( coro);
    int const& res = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( & a, & res);
    coro( a);
    BOOST_CHECK( ! coro);
}

void test_const_ref()
{
    int a = 3;
    coro_const_ref coro( f101, a);
    BOOST_CHECK( coro);
    int const& res = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( & a, & res);
    coro( a);
    BOOST_CHECK( ! coro);
}

void test_tuple()
{
    int a = 3, b = 7;
    coro_tuple coro( f11, coro_tuple::arguments( a, b) );
    BOOST_CHECK( coro);
    boost::tuple<int&,int&> tpl = coro.get();
    BOOST_CHECK( coro);
    BOOST_CHECK_EQUAL( & a, & tpl.get< 0 >() );
    BOOST_CHECK_EQUAL( & b, & tpl.get< 1 >() );
    coro( a, b);
    BOOST_CHECK( ! coro);
}

void test_unwind()
{
    value1 = 0;
    {
        BOOST_CHECK_EQUAL( ( int) 0, value1);
        coro_int coro( f12, coro_int::arguments( 3, 7) );
        BOOST_CHECK( coro);
        int res = coro.get();
        BOOST_CHECK_EQUAL( ( int) 7, value1);
        BOOST_CHECK( coro);
        BOOST_CHECK_EQUAL( ( int) 10, res);
    }
    BOOST_CHECK_EQUAL( ( int) 0, value1);
}

void test_no_unwind()
{
    value1 = 0;
    {
        BOOST_CHECK_EQUAL( ( int) 0, value1);
        coro_int coro(
            f12,
            coro_int::arguments( 3, 7),
            coro::attributes(
                coro::stack_allocator::default_stacksize(),
                coro::no_stack_unwind) );
        BOOST_CHECK( coro);
        int res = coro.get();
        BOOST_CHECK( coro);
        BOOST_CHECK_EQUAL( ( int) 10, res);
    }
    BOOST_CHECK_EQUAL( ( int) 7, value1);
}

void test_exceptions()
{
    bool thrown = false;
    std::runtime_error ex("abc");
    try
    {
        coro_void_void coro( boost::bind( f14< std::runtime_error >, _1, ex) );
        BOOST_CHECK( ! coro);
        BOOST_CHECK( false);
    }
    catch ( std::runtime_error const&)
    { thrown = true; }
    catch ( std::exception const&)
    {}
    catch (...)
    {}
    BOOST_CHECK( thrown);
}

void test_output_iterator()
{
    {
        std::vector< int > vec;
        coro_int_void coro( f16);
        BOOST_FOREACH( int i, coro)
        { vec.push_back( i); }
        BOOST_CHECK_EQUAL( ( std::size_t)5, vec.size() );
        BOOST_CHECK_EQUAL( ( int)1, vec[0] );
        BOOST_CHECK_EQUAL( ( int)2, vec[1] );
        BOOST_CHECK_EQUAL( ( int)3, vec[2] );
        BOOST_CHECK_EQUAL( ( int)4, vec[3] );
        BOOST_CHECK_EQUAL( ( int)5, vec[4] );
    }
    {
        std::vector< int > vec;
        coro_int_void coro( f16);
        coro_int_void::iterator e = boost::end( coro);
        for (
            coro_int_void::iterator i = boost::begin( coro);
            i != e; ++i)
        { vec.push_back( * i); }
        BOOST_CHECK_EQUAL( ( std::size_t)5, vec.size() );
        BOOST_CHECK_EQUAL( ( int)1, vec[0] );
        BOOST_CHECK_EQUAL( ( int)2, vec[1] );
        BOOST_CHECK_EQUAL( ( int)3, vec[2] );
        BOOST_CHECK_EQUAL( ( int)4, vec[3] );
        BOOST_CHECK_EQUAL( ( int)5, vec[4] );
    }
    {
        int i1 = 1, i2 = 2, i3 = 3;
        std::vector< const int* > vec_in;
        vec_in.push_back( & i1);
        vec_in.push_back( & i2);
        vec_in.push_back( & i3);
        std::vector< const int* > vec_out;
        coro_const_int_ptr_void coro( boost::bind( f19, _1, boost::ref( vec_in) ) );
        coro_const_int_ptr_void::const_iterator e = boost::const_end( coro);
        for (
            coro_const_int_ptr_void::const_iterator i = boost::const_begin( coro);
            i != e; ++i)
        { vec_out.push_back( * i); }
        BOOST_CHECK_EQUAL( ( std::size_t)3, vec_out.size() );
        BOOST_CHECK_EQUAL( & i1, vec_out[0] );
        BOOST_CHECK_EQUAL( & i2, vec_out[1] );
        BOOST_CHECK_EQUAL( & i3, vec_out[2] );
    }
}

void test_input_iterator()
{
    int counter = 0;
    std::vector< int > vec;
    coro_void_int coro(
        boost::bind( f17, _1, boost::ref( vec) ),
        counter);
    coro_void_int::iterator e( boost::end( coro) );
    for ( coro_void_int::iterator i( boost::begin( coro) );
          i != e; ++i)
    {
        i = ++counter;
    }
    BOOST_CHECK_EQUAL( ( std::size_t)5, vec.size() );
    BOOST_CHECK_EQUAL( ( int)0, vec[0] );
    BOOST_CHECK_EQUAL( ( int)1, vec[1] );
    BOOST_CHECK_EQUAL( ( int)2, vec[2] );
    BOOST_CHECK_EQUAL( ( int)3, vec[3] );
    BOOST_CHECK_EQUAL( ( int)4, vec[4] );
}

void test_pre()
{
    coro_int_int coro( f18, 0);
    BOOST_CHECK( coro);
    int res = coro.get();
    BOOST_CHECK_EQUAL( ( int) 1, res);
    BOOST_CHECK( coro);
    coro( -1);
    BOOST_CHECK( ! coro);
}

void test_post()
{
    coro_int_int coro( f18);
    BOOST_CHECK( coro);
    int res = coro.get();
    BOOST_CHECK_EQUAL( ( int) -1, res);
    BOOST_CHECK( coro);
    coro( -1);
    BOOST_CHECK( ! coro);
}
#endif

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.coroutine: coroutine test suite");

    test->add( BOOST_TEST_CASE( & test_move) );
    test->add( BOOST_TEST_CASE( & test_complete) );
    test->add( BOOST_TEST_CASE( & test_jump) );
    test->add( BOOST_TEST_CASE( & test_result_int) );
    test->add( BOOST_TEST_CASE( & test_result_string) );
    test->add( BOOST_TEST_CASE( & test_arg_int) );
    test->add( BOOST_TEST_CASE( & test_arg_string) );
    test->add( BOOST_TEST_CASE( & test_fp) );
    test->add( BOOST_TEST_CASE( & test_ptr) );
    test->add( BOOST_TEST_CASE( & test_const_ptr) );
#ifndef BOOST_COROUTINES_UNIDIRECT
    test->add( BOOST_TEST_CASE( & test_pre) );
    test->add( BOOST_TEST_CASE( & test_post) );
#else
    test->add( BOOST_TEST_CASE( & test_invalid_result) );
#endif
    test->add( BOOST_TEST_CASE( & test_ref) );
    test->add( BOOST_TEST_CASE( & test_const_ref) );
    test->add( BOOST_TEST_CASE( & test_tuple) );
    test->add( BOOST_TEST_CASE( & test_unwind) );
    test->add( BOOST_TEST_CASE( & test_no_unwind) );
    test->add( BOOST_TEST_CASE( & test_exceptions) );
    test->add( BOOST_TEST_CASE( & test_output_iterator) );
    test->add( BOOST_TEST_CASE( & test_input_iterator) );

    return test;
}
