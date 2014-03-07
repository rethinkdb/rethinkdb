/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <string>

#include <boost/algorithm/searching/boyer_moore.hpp>
#include <boost/algorithm/searching/boyer_moore_horspool.hpp>
#include <boost/algorithm/searching/knuth_morris_pratt.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( test_main )
{
    const std::string cs;
    std::string estr;
    std::string str ( "abc" );
    
//  empty corpus, empty pattern
    BOOST_CHECK ( 
        boost::algorithm::boyer_moore_search (
            cs.begin (), cs.end (), estr.begin (), estr.end ())
        == cs.begin ()
        );

    BOOST_CHECK ( 
        boost::algorithm::boyer_moore_horspool_search (
            cs.begin (), cs.end (), estr.begin (), estr.end ())
        == cs.begin ()
        );

    BOOST_CHECK ( 
        boost::algorithm::knuth_morris_pratt_search (
            cs.begin (), cs.end (), estr.begin (), estr.end ())
        == cs.begin ()
        );

//  empty corpus, non-empty pattern
    BOOST_CHECK ( 
        boost::algorithm::boyer_moore_search (
            estr.begin (), estr.end (), str.begin (), str.end ())
        == estr.end ()
        );

    BOOST_CHECK ( 
        boost::algorithm::boyer_moore_horspool_search (
            estr.begin (), estr.end (), str.begin (), str.end ())
        == estr.end ()
        );

    BOOST_CHECK ( 
        boost::algorithm::knuth_morris_pratt_search (
            estr.begin (), estr.end (), str.begin (), str.end ())
        == estr.end ()
        );

//  non-empty corpus, empty pattern
    BOOST_CHECK ( 
        boost::algorithm::boyer_moore_search (
            str.begin (), str.end (), estr.begin (), estr.end ())
        == str.begin ()
        );

    BOOST_CHECK ( 
        boost::algorithm::boyer_moore_horspool_search (
            str.begin (), str.end (), estr.begin (), estr.end ())
        == str.begin ()
        );

    BOOST_CHECK ( 
        boost::algorithm::knuth_morris_pratt_search (
            str.begin (), str.end (), estr.begin (), estr.end ())
        == str.begin ()
        );
}
