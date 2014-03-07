//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/get_error_info.hpp>
#include <boost/exception/info_tuple.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/workaround.hpp>

struct throws_on_copy;
struct non_printable { };

struct
user_data
    {
    int & count;

    explicit
    user_data( int & count ):
        count(count)
        {
        ++count;
        }

    user_data( user_data const & x ):
        count(x.count)
        {
        ++count;
        }

    ~user_data()
        {
        --count;
        }
    };

typedef boost::error_info<struct tag_test_1,int> test_1;
typedef boost::error_info<struct tag_test_2,unsigned int> test_2;
typedef boost::error_info<struct tag_test_3,float> test_3;
typedef boost::error_info<struct tag_test_4,throws_on_copy> test_4;
typedef boost::error_info<struct tag_test_5,std::string> test_5;
typedef boost::error_info<struct tag_test_6,non_printable> test_6;
typedef boost::error_info<struct tag_user_data,user_data> test_7;

struct
test_exception:
    boost::exception
    {
    };

struct
throws_on_copy
    {
    throws_on_copy()
        {
        }

    throws_on_copy( throws_on_copy const & )
        {
        throw test_exception();
        }
    };

void
basic_test()
    {
    try
        {
        test_exception x;
        x << test_1(1) << test_2(2u) << test_3(3.14159f);
        throw x;
        }
    catch(
    test_exception & x )
        {
        ++*boost::get_error_info<test_1>(x);
        ++*boost::get_error_info<test_2>(x);
        ++*boost::get_error_info<test_3>(x);
        BOOST_TEST(*boost::get_error_info<test_1>(x)==2);
        BOOST_TEST(*boost::get_error_info<test_2>(x)==3u);
        BOOST_TEST(*boost::get_error_info<test_3>(x)==4.14159f);
        BOOST_TEST(!boost::get_error_info<test_4>(x));
        }
    try
        {
        test_exception x;
        x << test_1(1) << test_2(2u) << test_3(3.14159f);
        throw x;
        }
    catch(
    test_exception const & x )
        {
        BOOST_TEST(*boost::get_error_info<test_1>(x)==1);
        BOOST_TEST(*boost::get_error_info<test_2>(x)==2u);
        BOOST_TEST(*boost::get_error_info<test_3>(x)==3.14159f);
        BOOST_TEST(!boost::get_error_info<test_4>(x));
        }
    }

void
exception_safety_test()
    {
    test_exception x;
    try
        {
        x << test_4(throws_on_copy());
        BOOST_TEST(false);
        }
    catch(
    test_exception & )
        {
        BOOST_TEST(!boost::get_error_info<test_4>(x));
        }
    }

void
throw_empty()
    {
    throw test_exception();
    }

void
throw_test_1( char const * value )
    {
    throw test_exception() << test_5(std::string(value));
    }

void
throw_test_2()
    {
    throw test_exception() << test_6(non_printable());
    }

void
throw_catch_add_file_name( char const * name )
    {
    try
        {
        throw_empty();
        BOOST_TEST(false);
        }
    catch(
    boost::exception & x )
        {
        x << test_5(std::string(name));
        throw;
        }     
    }

void
test_empty()
    {
    try
        {
        throw_empty();
        BOOST_TEST(false);
        }
    catch(
    boost::exception & x )
        {
#ifndef BOOST_NO_RTTI
        BOOST_TEST( dynamic_cast<test_exception *>(&x) );
#endif
        BOOST_TEST( !boost::get_error_info<test_1>(x) );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }

    try
        {
        throw_empty();
        BOOST_TEST(false);
        }
    catch(
    test_exception & x )
        {
#ifndef BOOST_NO_RTTI
        BOOST_TEST( dynamic_cast<boost::exception const *>(&x)!=0 );
#endif
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    }

void
test_basic_throw_catch()
    {
    try
        {
        throw_test_1("test");
        BOOST_ASSERT(false);
        }
    catch(
    boost::exception & x )
        {
        BOOST_TEST(*boost::get_error_info<test_5>(x)==std::string("test"));
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }

    try
        {
        throw_test_2();
        BOOST_ASSERT(false);
        }
    catch(
    boost::exception & x )
        {
        BOOST_TEST(boost::get_error_info<test_6>(x));
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    }

void
test_catch_add_info()
    {
    try
        {
        throw_catch_add_file_name("test");
        BOOST_TEST(false);
        }
    catch(
    boost::exception & x )
        {
        BOOST_TEST(*boost::get_error_info<test_5>(x)==std::string("test"));
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    }

void
test_add_tuple()
    {
    typedef boost::tuple<> tuple_test_;
    typedef boost::tuple<test_1> tuple_test_1;
    typedef boost::tuple<test_1,test_2> tuple_test_12;
    typedef boost::tuple<test_1,test_2,test_3> tuple_test_123;
    typedef boost::tuple<test_1,test_2,test_3,test_5> tuple_test_1235;
    try
        {
        throw test_exception() << tuple_test_();
        }
    catch(
    test_exception & x )
        {
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    try
        {
        throw test_exception() << tuple_test_1(42);
        }
    catch(
    test_exception & x )
        {
        BOOST_TEST( *boost::get_error_info<test_1>(x)==42 );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    try
        {
        throw test_exception() << tuple_test_12(42,42u);
        }
    catch(
    test_exception & x )
        {
        BOOST_TEST( *boost::get_error_info<test_1>(x)==42 );
        BOOST_TEST( *boost::get_error_info<test_2>(x)==42u );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    try
        {
        throw test_exception() << tuple_test_123(42,42u,42.0f);
        }
    catch(
    test_exception & x )
        {
        BOOST_TEST( *boost::get_error_info<test_1>(x)==42 );
        BOOST_TEST( *boost::get_error_info<test_2>(x)==42u );
        BOOST_TEST( *boost::get_error_info<test_3>(x)==42.0f );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    try
        {
        throw test_exception() << tuple_test_1235(42,42u,42.0f,std::string("42"));
        }
    catch(
    test_exception & x )
        {
        BOOST_TEST( *boost::get_error_info<test_1>(x)==42 );
        BOOST_TEST( *boost::get_error_info<test_2>(x)==42u );
        BOOST_TEST( *boost::get_error_info<test_3>(x)==42.0f );
        BOOST_TEST( *boost::get_error_info<test_5>(x)=="42" );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    }

void
test_lifetime1()
    {
    int count=0;
    try
        {
        throw test_exception() << test_7(user_data(count));
        }
    catch(
    boost::exception & x )
        {
        BOOST_TEST(count==1);
        BOOST_TEST( boost::get_error_info<test_7>(x) );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    BOOST_TEST(!count);
    }

void
test_lifetime2()
    {
    int count=0;
        {
        boost::exception_ptr ep;
        test_exception e; e<<test_7(user_data(count));
        ep=boost::copy_exception(e);
        BOOST_TEST(count>0);
        }
    BOOST_TEST(!count);
    }

bool
is_const( int const * )
    {
    return true;
    }

bool
is_const( int * )
    {
    return false;
    }

void
test_const()
    {
    test_exception e;
    boost::exception const & c(e);
    boost::exception & m(e);
    BOOST_TEST(is_const(boost::get_error_info<test_1>(c)));
    BOOST_TEST(!is_const(boost::get_error_info<test_1>(m)));
    }

int
main()
    {
    basic_test();
    exception_safety_test();
    test_empty();
    test_basic_throw_catch();
    test_catch_add_info();
    test_add_tuple();
    test_lifetime1();
    test_lifetime2();
    test_const();
    return boost::report_errors();
    }
