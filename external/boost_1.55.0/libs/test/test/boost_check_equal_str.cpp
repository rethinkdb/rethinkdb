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
//  Description : basic_cstring unit test
// *****************************************************************************

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( check_string_compare )
{
    char const* buf_ptr_cch     = "abc";
    char const  buf_array_cch[] = "abc";
    char        buf_array_ch[]  = "abc";
    char*       buf_ptr_ch      = buf_array_ch;
    std::string buf_str         = "abc";

    BOOST_CHECK_EQUAL(buf_ptr_cch, buf_ptr_cch);
    BOOST_CHECK_EQUAL(buf_ptr_cch, buf_array_cch);
    BOOST_CHECK_EQUAL(buf_ptr_cch, buf_ptr_ch);
    BOOST_CHECK_EQUAL(buf_ptr_cch, buf_array_ch);
    BOOST_CHECK_EQUAL(buf_ptr_cch, buf_str);

    BOOST_CHECK_EQUAL(buf_array_cch, buf_ptr_cch);
    BOOST_CHECK_EQUAL(buf_array_cch, buf_array_cch);
    BOOST_CHECK_EQUAL(buf_array_cch, buf_ptr_ch);
    BOOST_CHECK_EQUAL(buf_array_cch, buf_array_ch);
    BOOST_CHECK_EQUAL(buf_array_cch, buf_str);

    BOOST_CHECK_EQUAL(buf_ptr_ch, buf_ptr_cch);
    BOOST_CHECK_EQUAL(buf_ptr_ch, buf_array_cch);
    BOOST_CHECK_EQUAL(buf_ptr_ch, buf_ptr_ch);
    BOOST_CHECK_EQUAL(buf_ptr_ch, buf_array_ch);
    BOOST_CHECK_EQUAL(buf_ptr_ch, buf_str);

    BOOST_CHECK_EQUAL(buf_array_ch, buf_ptr_cch);
    BOOST_CHECK_EQUAL(buf_array_ch, buf_array_cch);
    BOOST_CHECK_EQUAL(buf_array_ch, buf_ptr_ch);
    BOOST_CHECK_EQUAL(buf_array_ch, buf_array_ch);
    BOOST_CHECK_EQUAL(buf_array_ch, buf_str);

    BOOST_CHECK_EQUAL(buf_str, buf_ptr_cch);
    BOOST_CHECK_EQUAL(buf_str, buf_array_cch);
    BOOST_CHECK_EQUAL(buf_str, buf_ptr_ch);
    BOOST_CHECK_EQUAL(buf_str, buf_array_ch);
    BOOST_CHECK_EQUAL(buf_str, buf_str);
}

//____________________________________________________________________________//

// EOF
