/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <boost/algorithm/searching/boyer_moore.hpp>
#include <boost/algorithm/searching/boyer_moore_horspool.hpp>
#include <boost/algorithm/searching/knuth_morris_pratt.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <ctime>        // for clock_t
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <string>

typedef std::vector<std::string> vec;
#define NUM_TRIES   100

#define runOne(call, refDiff)   { \
    std::clock_t bTime, eTime;                              \
    bTime = std::clock ();                                  \
    for ( i = 0; i < NUM_TRIES; ++i ) {                     \
        res = boost::algorithm::call                        \
            ( haystack.begin (), haystack.end (),           \
                        needle.begin (), needle.end ());    \
        if ( res != exp ) {                                 \
            std::cout << "On run # " << i << " expected "   \
                << exp - haystack.begin () << " got "       \
                << res - haystack.begin () << std::endl;    \
            throw std::runtime_error                        \
                ( "Unexpected result from " #call );        \
            }                                               \
        }                                                   \
    eTime = std::clock ();                                  \
    printRes ( #call, eTime - bTime, refDiff ); }
    
#define runObject(obj, refDiff) { \
    std::clock_t bTime, eTime;                              \
    bTime = std::clock ();                                  \
    boost::algorithm::obj <vec::const_iterator>             \
                s_o ( needle.begin (), needle.end ());      \
    for ( i = 0; i < NUM_TRIES; ++i ) {                     \
        res = s_o ( haystack.begin (), haystack.end ());    \
        if ( res != exp ) {                                 \
            std::cout << "On run # " << i << " expected "   \
            << exp - haystack.begin () << " got "           \
            << res - haystack.begin () << std::endl;        \
            throw std::runtime_error                        \
            ( "Unexpected result from " #obj " object" );   \
            }                                               \
        }                                                   \
    eTime = std::clock ();                                  \
    printRes ( #obj " object", eTime - bTime, refDiff ); }
    

namespace {

    vec ReadFromFile ( const char *name ) {
        std::ifstream in ( name, std::ios_base::binary | std::ios_base::in );
        std::string temp;
        vec retVal;
        while ( std::getline ( in, temp ))
            retVal.push_back ( temp );
        
        return retVal;
        }
    
    void printRes ( const char *prompt, unsigned long diff, unsigned long stdDiff ) {
        std::cout 
            << std::setw(34) << prompt << " "
            << std::setw(6)  << (  1.0 * diff) / CLOCKS_PER_SEC << " seconds\t"
            << std::setw(5)  << (100.0 * diff) / stdDiff << "% \t" 
            << std::setw(12) << diff;
        if ( diff > stdDiff ) 
            std::cout << " !!";
        std::cout << std::endl;
        }
    
    void check_one ( const vec &haystack, const vec &needle, int expected ) {
        std::size_t i;
        std::clock_t sTime;
        unsigned long stdDiff;
        
        vec::const_iterator res;
        vec::const_iterator exp;        // the expected result
        
        if ( expected >= 0 )
            exp = haystack.begin () + expected;
        else if ( expected == -1 )
            exp = haystack.end ();      // we didn't find it1
        else if ( expected == -2 )
            exp = std::search ( haystack.begin (), haystack.end (), needle.begin (), needle.end ());
        else    
            throw std::logic_error ( "Expected must be -2, -1, or >= 0" );

        std::cout << "Pattern is " << needle.size ()   << " entries long" << std::endl;
        std::cout << "Corpus  is " << haystack.size () << " entries long" << std::endl;

    //  First, the std library search
        sTime = std::clock ();
        for ( i = 0; i < NUM_TRIES; ++i ) {
            res = std::search ( haystack.begin (), haystack.end (), needle.begin (), needle.end ());
            if ( res != exp ) {
                std::cout << "On run # " << i << " expected " << exp - haystack.begin () << " got " << res - haystack.begin () << std::endl;
                throw std::runtime_error ( "Unexpected result from std::search" );
                }
            }
        stdDiff = std::clock () - sTime;
        printRes ( "std::search", stdDiff, stdDiff );

        runOne    ( boyer_moore_search,          stdDiff );
        runObject ( boyer_moore,                 stdDiff );
        runOne    ( boyer_moore_horspool_search, stdDiff );
        runObject ( boyer_moore_horspool,        stdDiff );
        runOne    ( knuth_morris_pratt_search,   stdDiff );
        runObject ( knuth_morris_pratt,          stdDiff );
        }
    }

BOOST_AUTO_TEST_CASE( test_main )
{
    vec c1  = ReadFromFile ( "search_test_data/0001.corpus" );
    vec p1b = ReadFromFile ( "search_test_data/0002b.pat" );
    vec p1e = ReadFromFile ( "search_test_data/0002e.pat" );
    vec p1n = ReadFromFile ( "search_test_data/0002n.pat" );
    vec p1f = ReadFromFile ( "search_test_data/0002f.pat" );

    std::cout << std::ios::fixed << std::setprecision(4);
//  std::cout << "Corpus is " << c1.size () << " entries long\n";
    std::cout << "--- Beginning ---" << std::endl;
    check_one ( c1, p1b, 0 );       //  Find it at position zero
    std::cout << "---- Middle -----" << std::endl;
    check_one ( c1, p1f, -2 );      //  Don't know answer
    std::cout << "------ End ------" << std::endl;
    check_one ( c1, p1e, c1.size() - p1e.size ());  
    std::cout << "--- Not found ---" << std::endl;
    check_one ( c1, p1n, -1 );      //  Not found
    }
