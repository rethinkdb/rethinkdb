//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : basic_cstring unit test
// *****************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
namespace utf = boost::unit_test;

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/file/config_file_iterator.hpp>
#include <boost/test/utils/runtime/env/variable.hpp>

namespace rt  = boost::runtime;
namespace file = boost::runtime::file;
namespace env = boost::runtime::environment;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_constructor )
{
    {
    file::config_file_iterator cfi( NULL );

    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "" );

    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    rt::cstring cs( "" );
    file::config_file_iterator cfi( cs );

    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    std::string ds;
    file::config_file_iterator cfi( ds );

    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    BOOST_CHECK_THROW( file::config_file_iterator( "!@#%#$%#$^#$^" ), rt::logic_error );
    }

    {
    file::config_file_iterator cfi( "test_files/test_constructor.cfg" );

    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "{ abc d }" );

    cfi = cfi;

    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "{ abc d }" );

    file::config_file_iterator cfi1( cfi );

    BOOST_CHECK( cfi == file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi1, "{ abc d }" );

    ++cfi1;
    BOOST_CHECK_EQUAL( *cfi1, "{ d" );

    cfi = cfi1;
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "{ d" );

    ++cfi;
    BOOST_CHECK( *cfi == " dsfg" );
    }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_comments_and_blanks )
{
    file::config_file_iterator cfi( "test_files/test_comments_and_blanks.cfg" );

    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "1" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "2" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "4" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "3" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_broken_line )
{
    BOOST_CHECK_THROW( file::config_file_iterator( "test_files/test_incomplete_broken_line.cfg" ), rt::logic_error );

    {
    file::config_file_iterator cfi( "test_files/test_broken_line.cfg" );

    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "qwerty" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "123 \\11" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "   23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "xcv \\ dfgsd" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "qwe" ); ++cfi;
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "1 \t23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "34 34" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "a b c d e f" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "as sa" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "aswe" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "test_files/test_broken_line.cfg", file::trim_leading_spaces );

    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "qwerty" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "123 \\11" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "xcv \\ dfgsd" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "qwe" ); ++cfi;
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "1 \t23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "34 34" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "a b c d e f" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "as sa" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "aswe" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "test_files/test_broken_line.cfg", (!file::trim_leading_spaces,!file::trim_trailing_spaces));

    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "qwerty" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "123 \\11" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "   23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "xcv \\ dfgsd" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "qwe" ); ++cfi;
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "1  " ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "\t23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "34 \\  " ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "34" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "a b c d e f " ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "as \\ " ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "sa" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "aswe" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "test_files/test_broken_line.cfg", !file::skip_empty_lines );

    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "qwerty" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "123 \\11" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "   23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "xcv \\ dfgsd" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "qwe" ); ++cfi;
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "1 " ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "\t23" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "34 34" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "a b c d e f" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "as " ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "sa" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "as" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "we" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_include )
{
    {
    file::config_file_iterator cfi( "test_files/test_include1.cfg" );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "a" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "c" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "b" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "test_files/test_include2.cfg" );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "1" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "a" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "c" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "b" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "2" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "test_files/test_include3.cfg" );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "c" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "c" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }
}

//____________________________________________________________________________//

env::variable<> TEST_MACRO( "TEST_MACRO", env::default_value = "test_value" );

BOOST_AUTO_TEST_CASE( test_define )
{
    {
        file::config_file_iterator cfi( "test_files/test_define.cfg" );
        BOOST_CHECK( cfi != file::config_file_iterator() );
        BOOST_CHECK_EQUAL( *cfi, "a123123" ); ++cfi;
        BOOST_CHECK_EQUAL( *cfi, "11232" ); ++cfi;
        BOOST_CHECK_EQUAL( *cfi, "a test_value=11" ); ++cfi;
        BOOST_CHECK_EQUAL( *cfi, "1abc2" ); ++cfi;
        BOOST_CHECK( cfi == file::config_file_iterator() );
    }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_macro_subst )
{
    BOOST_CHECK_THROW( file::config_file_iterator( "test_files/test_macro_subst1.cfg" ), rt::logic_error );
    BOOST_CHECK_THROW( file::config_file_iterator( "test_files/test_macro_subst3.cfg" ), rt::logic_error );

    {
    file::config_file_iterator cfi( "test_files/test_macro_subst1.cfg", !file::detect_missing_macro );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "a" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "test_files/test_macro_subst2.cfg" );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "atest_value" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    {
    file::config_file_iterator cfi( "test_files/test_macro_subst4.cfg" );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "abb" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_undef )
{
    {
    file::config_file_iterator cfi( "test_files/test_undef.cfg", !file::detect_missing_macro );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "1123" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "1" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_ifdef )
{
    {
    file::config_file_iterator cfi( "test_files/test_ifdef.cfg" );
    BOOST_CHECK( cfi != file::config_file_iterator() );
    BOOST_CHECK_EQUAL( *cfi, "1" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "2" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "1" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "1abc" ); ++cfi;
    BOOST_CHECK_EQUAL( *cfi, "a" ); ++cfi;
    BOOST_CHECK( cfi == file::config_file_iterator() );
    }

    BOOST_CHECK_THROW( file::config_file_iterator( "test_files/test_ifdef1.cfg" ), rt::logic_error );
    BOOST_CHECK_THROW( file::config_file_iterator( "test_files/test_ifdef2.cfg" ), rt::logic_error );
    BOOST_CHECK_THROW( file::config_file_iterator( "test_files/test_ifdef3.cfg" ), rt::logic_error );
    BOOST_CHECK_THROW( file::config_file_iterator( "test_files/test_ifdef4.cfg" ), rt::logic_error );
}

//____________________________________________________________________________//

// EOF
