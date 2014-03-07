/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org

Test non-string cases; vector and list
*/

#include <boost/config.hpp>
#include <boost/algorithm/hex.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <deque>
#include <list>


const char *tohex [] = {
    "",
    "a",
    "\001",
    "12",
    "asdfadsfsad",
    "01234567890ABCDEF",
    NULL        // End of the list
    };

void test_to_hex () {
    for ( const char **p = tohex; *p; p++ ) {
        std::deque<char> arg, argh;
        std::list<char> one, two, three;
        arg.assign ( *p, *p + strlen (*p));
        boost::algorithm::hex ( *p,                       std::back_inserter ( one ));
        boost::algorithm::hex ( arg,                      std::back_inserter ( two ));
        boost::algorithm::hex ( arg.begin (), arg.end (), std::back_inserter ( three ));
        BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));

        std::copy ( one.begin (), one.end (), std::back_inserter ( argh ));
        one.clear (); two.clear (); three.clear ();

//      boost::algorithm::unhex ( argh.c_str (),              std::back_inserter ( one ));
        boost::algorithm::unhex ( argh,                       std::back_inserter ( two ));
        boost::algorithm::unhex ( argh.begin (), argh.end (), std::back_inserter ( three ));
//      BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), arg.begin ()));
        }

//  Again, with a front_inserter
    for ( const char **p = tohex; *p; p++ ) {
        std::deque<char> arg, argh;
        std::list<char> one, two, three;
        arg.assign ( *p, *p + strlen (*p));
        boost::algorithm::hex ( *p,                       std::front_inserter ( one ));
        boost::algorithm::hex ( arg,                      std::front_inserter ( two ));
        boost::algorithm::hex ( arg.begin (), arg.end (), std::front_inserter ( three ));
        BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));

    //  Copy, reversing
        std::copy ( one.begin (), one.end (), std::front_inserter ( argh ));
        one.clear (); two.clear (); three.clear ();

//      boost::algorithm::unhex ( argh.c_str (),              std::front_inserter ( one ));
        boost::algorithm::unhex ( argh,                       std::front_inserter ( two ));
        boost::algorithm::unhex ( argh.begin (), argh.end (), std::front_inserter ( three ));
//      BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), arg.rbegin ()));   // reverse
        }
    }

const char *fromhex [] = {
    "20",
    "2122234556FF",
    NULL        // End of the list
    };


void test_from_hex_success () {
    for ( const char **p = fromhex; *p; p++ ) {
        std::deque<char> arg, argh;
        std::list<char> one, two, three;
        arg.assign ( *p, *p + strlen (*p));
        boost::algorithm::unhex ( *p,                       std::back_inserter ( one ));
        boost::algorithm::unhex ( arg,                      std::back_inserter ( two ));
        boost::algorithm::unhex ( arg.begin (), arg.end (), std::back_inserter ( three ));
        BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));

        std::copy ( one.begin (), one.end (), std::back_inserter ( argh ));
        one.clear (); two.clear (); three.clear ();

//      boost::algorithm::hex ( argh.c_str (),              std::back_inserter ( one ));
        boost::algorithm::hex ( argh,                       std::back_inserter ( two ));
        boost::algorithm::hex ( argh.begin (), argh.end (), std::back_inserter ( three ));
//      BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), arg.begin ()));
        }

//  Again, with a front_inserter
    for ( const char **p = fromhex; *p; p++ ) {
        std::deque<char> arg, argh;
        std::list<char> one, two, three;
        arg.assign ( *p, *p + strlen (*p));
        boost::algorithm::unhex ( *p,                       std::front_inserter ( one ));
        boost::algorithm::unhex ( arg,                      std::front_inserter ( two ));
        boost::algorithm::unhex ( arg.begin (), arg.end (), std::front_inserter ( three ));
        BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));

    //  Copy, reversing
        std::copy ( one.begin (), one.end (), std::front_inserter ( argh ));
        one.clear (); two.clear (); three.clear ();

//      boost::algorithm::hex ( argh.c_str (),              std::front_inserter ( one ));
        boost::algorithm::hex ( argh,                       std::front_inserter ( two ));
        boost::algorithm::hex ( argh.begin (), argh.end (), std::front_inserter ( three ));
//      BOOST_CHECK ( std::equal ( one.begin (), one.end (), two.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), three.begin ()));
        BOOST_CHECK ( std::equal ( two.begin (), two.end (), arg.rbegin ()));   // reversed
        }
    }


BOOST_AUTO_TEST_CASE( test_main )
{
  test_to_hex ();
  test_from_hex_success ();
}
