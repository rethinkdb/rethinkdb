//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 57993 $
//
//  Description : tests all Test Tools but output_test_stream
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/detail/unit_test_parameters.hpp>
#include <boost/test/output/compiler_log_formatter.hpp>
#include <boost/test/framework.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

// Boost
#include <boost/bind.hpp>

// STL
#include <iostream>
#include <iomanip>

#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable: 4702) // unreachable code
#endif

using namespace boost::unit_test;
using namespace boost::test_tools;

//____________________________________________________________________________//

#define CHECK_CRITICAL_TOOL_USAGE( tool_usage )     \
{                                                   \
    bool throw_ = false;                            \
    try {                                           \
        tool_usage;                                 \
    } catch( boost::execution_aborted const& ) {    \
        throw_ = true;                              \
    }                                               \
                                                    \
    BOOST_CHECK_MESSAGE( throw_, "not aborted" );   \
}                                                   \
/**/

//____________________________________________________________________________//

class bool_convertible
{
    struct Tester {};
public:
    operator Tester*() const { return static_cast<Tester*>( 0 ) + 1; }
};

//____________________________________________________________________________//

struct shorten_lf : public boost::unit_test::output::compiler_log_formatter
{
    void    print_prefix( std::ostream& output, boost::unit_test::const_string, std::size_t line )
    {
        output << line << ": ";
    }
};

//____________________________________________________________________________//

std::string match_file_name( "./test_files/test_tools_test.pattern" );
std::string save_file_name( "test_tools_test.pattern" );

output_test_stream& ots()
{
    static boost::shared_ptr<output_test_stream> inst;

    if( !inst ) {
        inst.reset( 
            framework::master_test_suite().argc <= 1
                ? new output_test_stream( runtime_config::save_pattern() ? save_file_name : match_file_name, !runtime_config::save_pattern() )
                : new output_test_stream( framework::master_test_suite().argv[1], !runtime_config::save_pattern() ) );
    }

    return *inst;
}

//____________________________________________________________________________//

#define TEST_CASE( name )                                   \
void name ## _impl();                                       \
void name ## _impl_defer();                                 \
                                                            \
BOOST_AUTO_TEST_CASE( name )                                \
{                                                           \
    test_case* impl = make_test_case( &name ## _impl, #name ); \
                                                            \
    unit_test_log.set_stream( ots() );                      \
    unit_test_log.set_threshold_level( log_nothing );       \
    unit_test_log.set_formatter( new shorten_lf );          \
    framework::run( impl );                                 \
                                                            \
    unit_test_log.set_threshold_level(                      \
        runtime_config::log_level() != invalid_log_level    \
            ? runtime_config::log_level()                   \
            : log_all_errors );                             \
    unit_test_log.set_format( runtime_config::log_format());\
    unit_test_log.set_stream( std::cout );                  \
    BOOST_CHECK( ots().match_pattern() );                   \
}                                                           \
                                                            \
void name ## _impl()                                        \
{                                                           \
    unit_test_log.set_threshold_level( log_all_errors );    \
                                                            \
    name ## _impl_defer();                                  \
                                                            \
    unit_test_log.set_threshold_level( log_nothing );       \
}                                                           \
                                                            \
void name ## _impl_defer()                                  \
/**/

//____________________________________________________________________________//

TEST_CASE( test_BOOST_WARN )
{
    unit_test_log.set_threshold_level( log_warnings );
    BOOST_WARN( sizeof(int) == sizeof(short) );

    unit_test_log.set_threshold_level( log_successful_tests );
    BOOST_WARN( sizeof(unsigned char) == sizeof(char) );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_CHECK )
{
    // check correct behavior in if clause
    if( true )
        BOOST_CHECK( true );

    // check correct behavior in else clause
    if( false )
    {}
    else
        BOOST_CHECK( true );

    bool_convertible bc;
    BOOST_CHECK( bc );

    int i=2;

    BOOST_CHECK( false );
    BOOST_CHECK( 1==2 );
    BOOST_CHECK( i==1 );

    unit_test_log.set_threshold_level( log_successful_tests );
    BOOST_CHECK( i==2 );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_REQUIRE )
{
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE( true ) );

    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE( false ) );

    int j = 3;

    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE( j > 5 ) );

    unit_test_log.set_threshold_level( log_successful_tests );
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE( j < 5 ) );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_WARN_MESSAGE )
{
    BOOST_WARN_MESSAGE( sizeof(int) == sizeof(short), "memory won't be used efficiently" );
    int obj_size = 33;

    BOOST_WARN_MESSAGE( obj_size <= 8, 
                        "object size " << obj_size << " is too big to be efficiently passed by value" );

    unit_test_log.set_threshold_level( log_successful_tests );
    BOOST_WARN_MESSAGE( obj_size > 8, "object size " << obj_size << " is too small" );
}

//____________________________________________________________________________//

boost::test_tools::predicate_result
test_pred1()
{
    boost::test_tools::predicate_result res( false );

    res.message() << "Some explanation";

    return res;
}

TEST_CASE( test_BOOST_CHECK_MESSAGE )
{
    BOOST_CHECK_MESSAGE( 2+2 == 5, "Well, may be that what I believe in" );

    BOOST_CHECK_MESSAGE( test_pred1(), "Checking predicate failed" );

    unit_test_log.set_threshold_level( log_successful_tests );
    BOOST_CHECK_MESSAGE( 2+2 == 4, "Could it fail?" );

    int i = 1;
    int j = 2;
    std::string msg = "some explanation";
    BOOST_CHECK_MESSAGE( i > j, "Comparing " << i << " and " << j << ": " << msg );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_REQUIRE_MESSAGE )
{
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE_MESSAGE( false, "Here we should stop" ) );

    unit_test_log.set_threshold_level( log_successful_tests );
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE_MESSAGE( true, "That's OK" ) );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_ERROR )
{
    BOOST_ERROR( "Fail to miss an error" );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_FAIL )
{
    CHECK_CRITICAL_TOOL_USAGE( BOOST_FAIL( "No! No! Show must go on." ) );
}

//____________________________________________________________________________//

struct my_exception {
    explicit my_exception( int ec = 0 ) : m_error_code( ec ) {}

    int m_error_code;
};

bool is_critical( my_exception const& ex ) { return ex.m_error_code < 0; }

TEST_CASE( test_BOOST_CHECK_THROW )
{
    int i=0;
    BOOST_CHECK_THROW( i++, my_exception );

    unit_test_log.set_threshold_level( log_warnings );
    BOOST_WARN_THROW( i++, my_exception );

    unit_test_log.set_threshold_level( log_all_errors );
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE_THROW( i++, my_exception ) );

    unit_test_log.set_threshold_level( log_successful_tests );
    BOOST_CHECK_THROW( throw my_exception(), my_exception ); // unreachable code warning is expected
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_CHECK_EXCEPTION )
{
    BOOST_CHECK_EXCEPTION( throw my_exception( 1 ), my_exception, is_critical ); // unreachable code warning is expected

    unit_test_log.set_threshold_level( log_successful_tests );
    BOOST_CHECK_EXCEPTION( throw my_exception( -1 ), my_exception, is_critical ); // unreachable code warning is expected
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_CHECK_NO_THROW )
{
    int i=0;
    BOOST_CHECK_NO_THROW( i++ );

    BOOST_CHECK_NO_THROW( throw my_exception() ); // unreachable code warning is expected
}

//____________________________________________________________________________//

struct B {
    B( int i ) : m_i( i ) {}

    int m_i;
};

bool          operator==( B const& b1, B const& b2 ) { return b1.m_i == b2.m_i; }
std::ostream& operator<<( std::ostream& str, B const& b ) { return str << "B(" << b.m_i << ")"; }

//____________________________________________________________________________//

struct C {
    C( int i, int id ) : m_i( i ), m_id( id ) {}

    int m_i;
    int m_id;
};

boost::test_tools::predicate_result
operator==( C const& c1, C const& c2 )
{
    boost::test_tools::predicate_result res( c1.m_i == c2.m_i && c1.m_id == c2.m_id );

    if( !res ) {
        if( c1.m_i != c2.m_i )
            res.message() << "Index mismatch";
        else
            res.message() << "Id mismatch";
    }

    return res;
}

std::ostream& operator<<( std::ostream& str, C const& c ) { return str << "C(" << c.m_i << ',' << c.m_id << ")"; }

//____________________________________________________________________________//

TEST_CASE( test_BOOST_CHECK_EQUAL )
{
    int i=1;
    int j=2;
    BOOST_CHECK_EQUAL( i, j );
    BOOST_CHECK_EQUAL( ++i, j );
    BOOST_CHECK_EQUAL( i++, j );

    char const* str1 = "test1";
    char const* str2 = "test12";
    BOOST_CHECK_EQUAL( str1, str2 );

    unit_test_log.set_threshold_level( log_successful_tests );
    BOOST_CHECK_EQUAL( i+1, j );

    char const* str3 = "1test1";
    BOOST_CHECK_EQUAL( str1, str3+1 );

    unit_test_log.set_threshold_level( log_all_errors );
    str1 = NULL;
    str2 = NULL;
    BOOST_CHECK_EQUAL( str1, str2 );

    str1 = "test";
    str2 = NULL;
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE_EQUAL( str1, str2 ) );

    B b1(1);
    B b2(2);

    unit_test_log.set_threshold_level( log_warnings );
    BOOST_WARN_EQUAL( b1, b2 );

    unit_test_log.set_threshold_level( log_all_errors );
    C c1( 0, 100 );
    C c2( 0, 101 );
    C c3( 1, 102 );
    BOOST_CHECK_EQUAL( c1, c3 );
    BOOST_CHECK_EQUAL( c1, c2 );

    char ch1 = -2;
    char ch2 = -3;
    BOOST_CHECK_EQUAL(ch1, ch2);
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_CHECK_LOGICAL_EXPR )
{
    int i=1;
    int j=2;
    BOOST_CHECK_NE( i, j );

    BOOST_CHECK_NE( ++i, j );

    BOOST_CHECK_LT( i, j );
    BOOST_CHECK_GT( i, j );

    BOOST_CHECK_LE( i, j );
    BOOST_CHECK_GE( i, j );

    ++i;

    BOOST_CHECK_LE( i, j );
    BOOST_CHECK_GE( j, i );

    char const* str1 = "test1";
    char const* str2 = "test1";

    BOOST_CHECK_NE( str1, str2 );
}

//____________________________________________________________________________//

bool is_even( int i )        { return i%2 == 0;  }
int  foo( int arg, int mod ) { return arg % mod; }
bool moo( int arg1, int arg2, int mod ) { return ((arg1+arg2) % mod) == 0; }

BOOST_TEST_DONT_PRINT_LOG_VALUE( std::list<int> )

boost::test_tools::predicate_result
compare_lists( std::list<int> const& l1, std::list<int> const& l2 )
{
    if( l1.size() != l2.size() ) {
        boost::test_tools::predicate_result res( false );

        res.message() << "Different sizes [" << l1.size() << "!=" << l2.size() << "]";

        return res;
    }

    return true;
}

TEST_CASE( test_BOOST_CHECK_PREDICATE )
{
    BOOST_CHECK_PREDICATE( is_even, (14) );

    int i = 17;
    BOOST_CHECK_PREDICATE( is_even, (i) );

    using std::not_equal_to;
    BOOST_CHECK_PREDICATE( not_equal_to<int>(), (i)(17) );

    int j=15;
    BOOST_CHECK_PREDICATE( boost::bind( is_even, boost::bind( &foo, _1, _2 ) ), (i)(j) );

    unit_test_log.set_threshold_level( log_warnings );
    BOOST_WARN_PREDICATE( moo, (12)(i)(j) );

    unit_test_log.set_threshold_level( log_all_errors );
    std::list<int> l1, l2, l3;
    l1.push_back( 1 );
    l3.push_back( 1 );
    l1.push_back( 2 );
    l3.push_back( 3 );
    BOOST_CHECK_PREDICATE( compare_lists, (l1)(l2) );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_REQUIRE_PREDICATE )
{
    int arg1 = 1;
    int arg2 = 2;

    using std::less_equal;
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE_PREDICATE( less_equal<int>(), (arg1)(arg2) ) );

    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE_PREDICATE( less_equal<int>(), (arg2)(arg1) ) );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_CHECK_EQUAL_COLLECTIONS )
{
    unit_test_log.set_threshold_level( log_all_errors );

    int pattern [] = { 1, 2, 3, 4, 5, 6, 7 };

    std::list<int> testlist;

    testlist.push_back( 1 );
    testlist.push_back( 2 );
    testlist.push_back( 4 ); // 3
    testlist.push_back( 4 );
    testlist.push_back( 5 );
    testlist.push_back( 7 ); // 6
    testlist.push_back( 7 );

    BOOST_CHECK_EQUAL_COLLECTIONS( testlist.begin(), testlist.end(), pattern, pattern+7 );
    BOOST_CHECK_EQUAL_COLLECTIONS( testlist.begin(), testlist.end(), pattern, pattern+2 );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_CHECK_BITWISE_EQUAL )
{
    BOOST_CHECK_BITWISE_EQUAL( 0x16, 0x16 );

    BOOST_CHECK_BITWISE_EQUAL( (char)0x06, (char)0x16 );

    unit_test_log.set_threshold_level( log_warnings );
    BOOST_WARN_BITWISE_EQUAL( (char)0x26, (char)0x04 );

    unit_test_log.set_threshold_level( log_all_errors );
    CHECK_CRITICAL_TOOL_USAGE( BOOST_REQUIRE_BITWISE_EQUAL( (char)0x26, (int)0x26 ) );
}

//____________________________________________________________________________//

struct A {
    friend std::ostream& operator<<( std::ostream& str, A const& ) { str << "struct A"; return str;}
};

TEST_CASE( test_BOOST_TEST_MESSAGE )
{
    unit_test_log.set_threshold_level( log_messages );

    BOOST_TEST_MESSAGE( "still testing" );
    BOOST_TEST_MESSAGE( "1+1=" << 2 );

    int i = 2;
    BOOST_TEST_MESSAGE( i << "+" << i << "=" << (i+i) );

    A a = A();
    BOOST_TEST_MESSAGE( a );

#if BOOST_TEST_USE_STD_LOCALE
    BOOST_TEST_MESSAGE( std::hex << std::showbase << 20 );
#else
    BOOST_TEST_MESSAGE( "0x14" );
#endif

    BOOST_TEST_MESSAGE( std::setw( 4 ) << 20 );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_TEST_CHECKPOINT )
{
    BOOST_TEST_CHECKPOINT( "Going to do a silly things" );

    throw "some error";
}

//____________________________________________________________________________//

bool foo() { throw 1; return true; }

TEST_CASE( test_BOOST_TEST_PASSPOINT )
{
    BOOST_CHECK( foo() );
}

//____________________________________________________________________________//

TEST_CASE( test_BOOST_IS_DEFINED )
{
#define SYMBOL1
#define SYMBOL2 std::cout
#define ONE_ARG( arg ) arg
#define TWO_ARG( arg1, arg2 ) BOOST_JOIN( arg1, arg2 )

    BOOST_CHECK( BOOST_IS_DEFINED(SYMBOL1) );
    BOOST_CHECK( BOOST_IS_DEFINED(SYMBOL2) );
    BOOST_CHECK( !BOOST_IS_DEFINED( SYMBOL3 ) );
    BOOST_CHECK( BOOST_IS_DEFINED( ONE_ARG(arg1) ) );
    BOOST_CHECK( !BOOST_IS_DEFINED( ONE_ARG1(arg1) ) );
    BOOST_CHECK( BOOST_IS_DEFINED( TWO_ARG(arg1,arg2) ) );
    BOOST_CHECK( !BOOST_IS_DEFINED( TWO_ARG1(arg1,arg2) ) );
}

//____________________________________________________________________________//

// !! CHECK_SMALL

// EOF
