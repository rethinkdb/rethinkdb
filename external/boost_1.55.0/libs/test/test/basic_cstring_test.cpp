//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 54633 $
//
//  Description : basic_cstring unit test
// *****************************************************************************

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#pragma warning(disable: 4267)
#endif

// Boost.Test
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/test/utils/basic_cstring/compare.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/test/output_test_stream.hpp>
namespace utf = boost::unit_test;
namespace tt  = boost::test_tools;
using utf::const_string;

// Boost
#include <boost/mpl/list.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/type_traits/add_const.hpp>

// STL
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace mpl = boost::mpl;

#if defined(BOOST_NO_STD_WSTRING)
typedef mpl::list1<char const>                                              base_const_char_types;
typedef mpl::list2<char,unsigned char>                                      mutable_char_types;
#else
typedef mpl::list2<char const,wchar_t const>                                base_const_char_types;
typedef mpl::list3<char,unsigned char,wchar_t>                              mutable_char_types;
#endif
typedef mpl::transform<mutable_char_types,boost::add_const<mpl::_1> >::type const_char_types;
typedef mpl::joint_view<const_char_types,mutable_char_types>                char_types;
typedef mpl::list2<char,const char>                                         io_test_types;

//____________________________________________________________________________//

template<typename CharT>
struct string_literal
{
    typedef typename boost::remove_const<CharT>::type   mutable_char;

    string_literal( char const* orig, std::size_t orig_size )
    {
        std::copy( orig, orig + orig_size, buff );

        buff[orig_size] = 0;
    }

    mutable_char buff[100];
};

#define LITERAL( s ) (CharT*)string_literal<CharT>( s, sizeof( s ) ).buff
#define LOCAL_DEF( name, s )                                                \
string_literal<CharT> BOOST_JOIN( sl, __LINE__)(s, sizeof( s ));            \
utf::basic_cstring<CharT> name( (CharT*)(BOOST_JOIN( sl, __LINE__).buff) )  \
/**/

#define TEST_STRING test_string<CharT>( (CharT*)0 )

template<typename CharT>
CharT*
test_string( CharT* = 0 )
{
    static string_literal<CharT> l( "test_string", 11 );

    return l.buff;
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( constructors_test, CharT )
{
    {
        utf::basic_cstring<CharT> bcs;
        BOOST_CHECK_EQUAL( bcs.size(), (unsigned)0 );
        BOOST_CHECK( bcs.is_empty() );
    }

    {
        utf::basic_cstring<CharT> bcs( utf::basic_cstring<CharT>::null_str() );
        BOOST_CHECK_EQUAL( bcs.size(), (unsigned)0 );
        BOOST_CHECK( bcs.is_empty() );
    }

    {
        utf::basic_cstring<CharT> bcs( 0 );
        BOOST_CHECK_EQUAL( bcs.size(), (unsigned)0 );
        BOOST_CHECK( bcs.is_empty() );
    }

    {
        typedef typename utf::basic_cstring<CharT>::traits_type traits;

        utf::basic_cstring<CharT> bcs( TEST_STRING );
        BOOST_CHECK_EQUAL( traits::compare( bcs.begin(), TEST_STRING, bcs.size() ), 0 );
        BOOST_CHECK_EQUAL( bcs.size(), traits::length( TEST_STRING ) );

        utf::basic_cstring<CharT> bcs1( bcs );
        BOOST_CHECK_EQUAL( traits::compare( bcs1.begin(), TEST_STRING, bcs1.size() ), 0 );
    }

    {
        typedef typename utf::basic_cstring<CharT>::traits_type traits;

        utf::basic_cstring<CharT> bcs( TEST_STRING, 4 );
        BOOST_CHECK_EQUAL( traits::compare( bcs.begin(), LITERAL( "test" ), bcs.size() ), 0 );
    }

    {
        typedef typename utf::basic_cstring<CharT>::traits_type traits;

        utf::basic_cstring<CharT> bcs( TEST_STRING, TEST_STRING + 6 );
        BOOST_CHECK_EQUAL( traits::compare( bcs.begin(), LITERAL( "test_s" ), bcs.size() ), 0 );
    }
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( constructors_std_string_test, CharT )
{
    typedef typename utf::basic_cstring<CharT>::traits_type traits;

    {
        typename utf::basic_cstring<CharT>::std_string l( TEST_STRING );

        utf::basic_cstring<CharT> bcs( l );
        BOOST_CHECK_EQUAL( traits::compare( bcs.begin(), TEST_STRING, bcs.size() ), 0 );
    }

}

//____________________________________________________________________________//

void array_construction_test()
{
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x600))
    const_string bcs_array[] = { "str1", "str2" };

    BOOST_CHECK_EQUAL( const_string::traits_type::compare( bcs_array[0].begin(), "str1", bcs_array[0].size() ), 0 );
    BOOST_CHECK_EQUAL( const_string::traits_type::compare( bcs_array[1].begin(), "str2", bcs_array[1].size() ), 0 );

    const_string bcs( "abc" );
    BOOST_CHECK_EQUAL( const_string::traits_type::compare( bcs.begin(), "abc", bcs.size() ), 0 );
#endif
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( data_access_test, CharT )
{
    typedef typename utf::basic_cstring<CharT>::traits_type traits_type;

    utf::basic_cstring<CharT> bcs1( TEST_STRING );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), TEST_STRING, bcs1.size() ), 0 );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), bcs1.begin(), bcs1.size() ), 0 );

    BOOST_CHECK_EQUAL( bcs1[0], 't' );
    BOOST_CHECK_EQUAL( bcs1[4], '_' );
    BOOST_CHECK_EQUAL( bcs1[bcs1.size()-1], 'g' );

    BOOST_CHECK_EQUAL( bcs1[0], bcs1.at( 0 ) );
    BOOST_CHECK_EQUAL( bcs1[2], bcs1.at( 5 ) );
    BOOST_CHECK_EQUAL( bcs1.at( bcs1.size() - 1 ), 'g' );
    BOOST_CHECK_EQUAL( bcs1.at( bcs1.size() ), 0 );
    BOOST_CHECK_EQUAL( bcs1.at( bcs1.size()+1 ), 0 );

    BOOST_CHECK_EQUAL( utf::first_char( bcs1 ), 't' );
    BOOST_CHECK_EQUAL( utf::last_char( bcs1 ) , 'g' );

    BOOST_CHECK_EQUAL( utf::first_char( utf::basic_cstring<CharT>() ), 0 );
    BOOST_CHECK_EQUAL( utf::last_char( utf::basic_cstring<CharT>() ), 0 );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( size_test, CharT )
{
    utf::basic_cstring<CharT> bcs1;

    BOOST_CHECK_EQUAL( bcs1.size(), (unsigned)0 );
    BOOST_CHECK( bcs1.is_empty() );

    bcs1 = TEST_STRING;
    BOOST_CHECK_EQUAL( bcs1.size(), (unsigned)11 );

    bcs1.clear();
    BOOST_CHECK_EQUAL( bcs1.size(), (unsigned)0 );
    BOOST_CHECK( bcs1.is_empty() );

    bcs1 = utf::basic_cstring<CharT>( TEST_STRING, 4 );
    BOOST_CHECK_EQUAL( bcs1.size(), (unsigned)4 );

    bcs1.resize( 5 );
    BOOST_CHECK_EQUAL( bcs1.size(), (unsigned)4 );

    bcs1.resize( 3 );
    BOOST_CHECK_EQUAL( bcs1.size(), (unsigned)3 );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( asignment_test, CharT )
{
    typedef typename utf::basic_cstring<CharT>::traits_type traits_type;

    utf::basic_cstring<CharT> bcs1;
    string_literal<CharT> l( "test", 4 );

    bcs1 = l.buff;
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), LITERAL( "test" ), bcs1.size() ), 0 );

    utf::basic_cstring<CharT> bcs2( TEST_STRING );
    bcs1 = bcs2;
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), TEST_STRING, bcs1.size() ), 0 );

    bcs1.assign( l.buff );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), LITERAL( "test" ), bcs1.size() ), 0 );

    bcs1.assign( l.buff+1, l.buff+3 );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), LITERAL( "est" ), bcs1.size() ), 0 );

    bcs1.assign( bcs2, 4, 3 );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), LITERAL( "_st" ), bcs1.size() ), 0 );

    bcs1.swap( bcs2 );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), TEST_STRING, bcs1.size() ), 0 );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs2.begin(), LITERAL( "_st" ), bcs2.size() ), 0 );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( asignment_std_string_test, CharT )
{
    typedef typename utf::basic_cstring<CharT>::traits_type traits_type;

    utf::basic_cstring<CharT> bcs1;
    typename utf::basic_cstring<CharT>::std_string l( TEST_STRING );

    bcs1 = l;
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), TEST_STRING, bcs1.size() ), 0 );

    bcs1.assign( l );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), TEST_STRING, bcs1.size() ), 0 );

    bcs1.assign( l, 1, 2 );
    BOOST_CHECK_EQUAL( traits_type::compare( bcs1.begin(), LITERAL( "es" ), bcs1.size() ), 0 );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( comparison_test, CharT )
{
    utf::basic_cstring<CharT> bcs1( TEST_STRING );
    utf::basic_cstring<CharT> bcs2( TEST_STRING );

    BOOST_CHECK_EQUAL( bcs1, TEST_STRING );
    BOOST_CHECK_EQUAL( TEST_STRING, bcs1 );
    BOOST_CHECK_EQUAL( bcs1, bcs2 );

    bcs1.resize( 4 );

    BOOST_CHECK_EQUAL( bcs1, LITERAL( "test" ) );

    BOOST_CHECK( bcs1 != TEST_STRING );
    BOOST_CHECK( TEST_STRING != bcs1 );
    BOOST_CHECK( bcs1 != bcs2 );

    LOCAL_DEF( bcs3, "TeSt" );
    BOOST_CHECK( utf::case_ins_eq( bcs1, bcs3 ) );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( comparison_std_string_test, CharT )
{
    utf::basic_cstring<CharT> bcs1( TEST_STRING );
    typename utf::basic_cstring<CharT>::std_string l( TEST_STRING );

    BOOST_CHECK( bcs1 == l );
#if BOOST_WORKAROUND(BOOST_MSVC, > 1300)
    BOOST_CHECK( l == bcs1 );
#endif

    bcs1.resize( 4 );

    BOOST_CHECK( bcs1 != l );
#if BOOST_WORKAROUND(BOOST_MSVC, > 1300)
    BOOST_CHECK( l != bcs1 );
#endif
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( ordering_test, CharT )
{
    LOCAL_DEF( bcs1, "aBcd" );
    LOCAL_DEF( bcs2, "aBbdd" );
    LOCAL_DEF( bcs3, "aBbde" );
    LOCAL_DEF( bcs4, "abab" );

    BOOST_CHECK( bcs1 < bcs2 );
    BOOST_CHECK( bcs2 < bcs3 );
    BOOST_CHECK( bcs1 < bcs3 );
    BOOST_CHECK( bcs1 < bcs4 );

    utf::case_ins_less<CharT> cil;
    BOOST_CHECK( cil( bcs4, bcs1 ) );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( trim_test, CharT )
{
    LOCAL_DEF( bcs0, "tes" );

    bcs0.trim_right( 1 );
    BOOST_CHECK_EQUAL( bcs0.size(), (unsigned)2 );
    BOOST_CHECK_EQUAL( bcs0[0], 't' );

    bcs0.trim_left( 1 );
    BOOST_CHECK_EQUAL( bcs0.size(), (unsigned)1 );
    BOOST_CHECK_EQUAL( bcs0[0], 'e' );

    bcs0.trim_left( 1 );
    BOOST_CHECK( bcs0.is_empty() );

    bcs0 = TEST_STRING;
    bcs0.trim_left( 11 );
    BOOST_CHECK( bcs0.is_empty() );

    bcs0 = TEST_STRING;
    bcs0.trim_right( 11 );
    BOOST_CHECK( bcs0.is_empty() );

    bcs0 = TEST_STRING;
    bcs0.trim_right( bcs0.size() - bcs0.find( LITERAL( "t_s" ) ) - 3 );
    BOOST_CHECK_EQUAL( bcs0, LITERAL( "test_s" ) );

    bcs0.trim_left( bcs0.find( LITERAL( "t_s" ) ) );
    BOOST_CHECK_EQUAL( bcs0, LITERAL( "t_s" ) );

    LOCAL_DEF( bcs1, "abcd   " );
    LOCAL_DEF( bcs2, "     abcd" );
    LOCAL_DEF( bcs3, "  abcd  " );

    bcs1.trim_right();
    BOOST_CHECK_EQUAL( bcs1, LITERAL( "abcd" ) );

    bcs2.trim_left();
    BOOST_CHECK_EQUAL( bcs2, LITERAL( "abcd" ) );

    bcs3.trim( LITERAL( "\"" ) );
    BOOST_CHECK_EQUAL( bcs3, LITERAL( "  abcd  " ) );

    bcs3.trim();
    BOOST_CHECK_EQUAL( bcs3, LITERAL( "abcd" ) );

    bcs3.trim();
    BOOST_CHECK_EQUAL( bcs3, LITERAL( "abcd" ) );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( io_test, CharT )
{
    utf::basic_cstring<CharT> bcs1( TEST_STRING );
    bcs1.trim_right( 7 );

    tt::output_test_stream ostr;

    ostr << std::setw( 6 ) << bcs1;
    BOOST_CHECK( ostr.is_equal( "  test" ) );

    ostr << std::setw( 3 ) << bcs1;
    BOOST_CHECK( ostr.is_equal( "test" ) );

    ostr << std::setw( 5 ) << std::setiosflags( std::ios::left ) << bcs1;
    BOOST_CHECK( ostr.is_equal( "test " ) );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( find_test, CharT )
{
    typedef typename utf::basic_cstring<CharT>::size_type size;
    utf::basic_cstring<CharT> bcs1( TEST_STRING );

    size not_found = (size)utf::basic_cstring<CharT>::npos;

    BOOST_CHECK_EQUAL( bcs1.find( utf::basic_cstring<CharT>() ), not_found );
    BOOST_CHECK_EQUAL( bcs1.find( LITERAL( "test" ) ), (size)0 );
    BOOST_CHECK_EQUAL( bcs1.find( TEST_STRING ), (size)0 );
    BOOST_CHECK_EQUAL( bcs1.find( LITERAL( "test_string " ) ), not_found );
    BOOST_CHECK_EQUAL( bcs1.find( LITERAL( " test_string" ) ), not_found );
    BOOST_CHECK_EQUAL( bcs1.find( LITERAL( "est" ) ), (size)1 );
    BOOST_CHECK_EQUAL( bcs1.find( LITERAL( "t_st" ) ), (size)3 );
    BOOST_CHECK_EQUAL( bcs1.find( LITERAL( "ing" ) ), (size)8 );
    BOOST_CHECK_EQUAL( bcs1.find( LITERAL( "tst" ) ), not_found );

    BOOST_CHECK_EQUAL( bcs1.rfind( utf::basic_cstring<CharT>() ), not_found );
    BOOST_CHECK_EQUAL( bcs1.rfind( LITERAL( "test" ) ), (size)0 );
    BOOST_CHECK_EQUAL( bcs1.rfind( TEST_STRING ), (size)0 );
    BOOST_CHECK_EQUAL( bcs1.rfind( LITERAL( "test_string " ) ), not_found );
    BOOST_CHECK_EQUAL( bcs1.rfind( LITERAL( " test_string" ) ), not_found );
    BOOST_CHECK_EQUAL( bcs1.rfind( LITERAL( "est" ) ), (size)1 );
    BOOST_CHECK_EQUAL( bcs1.rfind( LITERAL( "t_st" ) ), (size)3 );
    BOOST_CHECK_EQUAL( bcs1.rfind( LITERAL( "ing" ) ), (size)8 );
    BOOST_CHECK_EQUAL( bcs1.rfind( LITERAL( "tst" ) ), not_found );
}

//____________________________________________________________________________//

void const_conversion()
{
    char arr[] = { "ABC" };

    utf::basic_cstring<char> str1( arr );
    utf::basic_cstring<char const> str2;

    str2.assign( str1 );

    BOOST_CHECK_EQUAL( str1, "ABC" );
    BOOST_CHECK_EQUAL( str2, "ABC" );
}

//____________________________________________________________________________//

utf::test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/[] )
{
    utf::test_suite* test= BOOST_TEST_SUITE("basic_cstring test");

    test->add( BOOST_TEST_CASE_TEMPLATE( constructors_test, char_types ) );
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x551))
    test->add( BOOST_TEST_CASE_TEMPLATE( constructors_std_string_test, base_const_char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( asignment_std_string_test, base_const_char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( comparison_std_string_test, base_const_char_types ) );
#endif
    test->add( BOOST_TEST_CASE( array_construction_test ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( data_access_test, char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( size_test, char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( asignment_test, char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( comparison_test, char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( ordering_test, char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( trim_test, char_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( io_test, io_test_types ) );
    test->add( BOOST_TEST_CASE_TEMPLATE( find_test, char_types ) );
    test->add( BOOST_TEST_CASE( &const_conversion ) );

    return test;
}

// EOF
