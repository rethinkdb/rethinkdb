/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org

Try ostream_iterators
*/

#include <boost/config.hpp>
#include <boost/algorithm/hex.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <deque>
#include <list>


template <typename char_type>
void test_to_hex ( const char_type ** tests ) {
    typedef std::basic_string<char_type> String;
    typedef std::basic_ostringstream<char_type> Stream;
    typedef std::ostream_iterator<char_type, char_type> Iter;

    for ( const char_type **p = tests; *p; p++ ) {
        String arg, argh;
        Stream one, two, three;
        arg.assign ( *p );
        boost::algorithm::hex ( *p, Iter ( one ));
        boost::algorithm::hex ( arg, Iter ( two ));
        boost::algorithm::hex ( arg.begin (), arg.end (), Iter ( three ));
        boost::algorithm::hex ( arg );
        BOOST_CHECK ( one.str () == two.str ());
        BOOST_CHECK ( one.str () == three.str ());
        argh = one.str ();
        one.str (String()); two.str (String()); three.str (String());
        boost::algorithm::unhex ( argh.c_str (), Iter ( one ));
        boost::algorithm::unhex ( argh, Iter ( two ));
        boost::algorithm::unhex ( argh.begin (), argh.end (), Iter ( three ));
        BOOST_CHECK ( one.str () == two.str ());
        BOOST_CHECK ( one.str () == three.str ());
        BOOST_CHECK ( one.str () == arg );
        }
    }


template <typename char_type>
void test_from_hex_success ( const char_type ** tests ) {
    typedef std::basic_string<char_type> String;
    typedef std::basic_ostringstream<char_type> Stream;
    typedef std::ostream_iterator<char_type, char_type> Iter;

    for ( const char_type **p = tests; *p; p++ ) {
        String arg, argh;
        Stream one, two, three;
        arg.assign ( *p );
        boost::algorithm::unhex ( *p,                       Iter ( one ));
        boost::algorithm::unhex ( arg,                      Iter ( two ));
        boost::algorithm::unhex ( arg.begin (), arg.end (), Iter ( three ));
        
        BOOST_CHECK ( one.str () == two.str ());
        BOOST_CHECK ( one.str () == three.str ());

        argh = one.str ();
        one.str (String()); two.str (String()); three.str (String());

        boost::algorithm::hex ( argh.c_str (),              Iter ( one ));
        boost::algorithm::hex ( argh,                       Iter ( two ));
        boost::algorithm::hex ( argh.begin (), argh.end (), Iter ( three ));

        BOOST_CHECK ( one.str () == two.str ());
        BOOST_CHECK ( one.str () == three.str ());
        BOOST_CHECK ( one.str () == arg );
        }

    }

const char *tohex [] = {
    "",
    "a",
    "\001",
    "12",
    "asdfadsfsad",
    "01234567890ABCDEF",
    NULL        // End of the list
    };

const wchar_t *tohex_w [] = {
    L"",
    L"a",
    L"\001",
    L"12",
    L"asdfadsfsad",
    L"01234567890ABCDEF",
    NULL        // End of the list
    };


const char *fromhex [] = {
    "20",
    "2122234556FF",
    NULL        // End of the list
    };

const wchar_t *fromhex_w [] = {
    L"11223320",
    L"21222345010256FF",
    NULL        // End of the list
    };



BOOST_AUTO_TEST_CASE( test_main )
{
  test_to_hex ( tohex );
  test_to_hex ( tohex_w );
  test_from_hex_success ( fromhex );
  test_from_hex_success ( fromhex_w );
}
