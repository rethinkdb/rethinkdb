/* 
   Copyright (c) Marshall Clow 2013.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <boost/config.hpp>
#include <boost/algorithm/cxx14/equal.hpp>

#include "iterator_test.hpp"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

template <typename T>
bool eq ( const T& a, const T& b ) { return a == b; }

template <typename T>
bool never_eq ( const T&, const T& ) { return false; }

int comparison_count = 0;
template <typename T>
bool counting_equals ( const T &a, const T &b ) {
    ++comparison_count;
    return a == b;
    }

namespace ba = boost::algorithm;

void test_equal ()
{
//  Note: The literal values here are tested against directly, careful if you change them:
    int num[] = { 1, 1, 2, 3, 5 };
    const int sz = sizeof (num)/sizeof(num[0]);
    
    
//  Empty sequences are equal to each other, but not to non-empty sequences
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num), 
                              input_iterator<int *>(num),     input_iterator<int *>(num)));
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num), 
                              input_iterator<int *>(num),     input_iterator<int *>(num),
                              never_eq<int> ));
    BOOST_CHECK ( ba::equal ( random_access_iterator<int *>(num),     random_access_iterator<int *>(num), 
                              random_access_iterator<int *>(num),     random_access_iterator<int *>(num),
                              never_eq<int> ));
                              
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num), 
                              input_iterator<int *>(num),     input_iterator<int *>(num + 1)));
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num + 1), input_iterator<int *>(num + 2), 
                              input_iterator<int *>(num),     input_iterator<int *>(num)));
    BOOST_CHECK (!ba::equal ( random_access_iterator<int *>(num + 1), random_access_iterator<int *>(num + 2), 
                              random_access_iterator<int *>(num),     random_access_iterator<int *>(num)));

//  Single element sequences are equal if they contain the same value
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              input_iterator<int *>(num),     input_iterator<int *>(num + 1)));
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              eq<int> ));
    BOOST_CHECK ( ba::equal ( random_access_iterator<int *>(num),     random_access_iterator<int *>(num + 1),
                              random_access_iterator<int *>(num),     random_access_iterator<int *>(num + 1),
                              eq<int> ));
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              never_eq<int> ));
    BOOST_CHECK (!ba::equal ( random_access_iterator<int *>(num),     random_access_iterator<int *>(num + 1),
                              random_access_iterator<int *>(num),     random_access_iterator<int *>(num + 1),
                              never_eq<int> ));

    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              input_iterator<int *>(num + 1), input_iterator<int *>(num + 2)));
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              input_iterator<int *>(num + 1), input_iterator<int *>(num + 2),
                              eq<int> ));

    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num + 2), input_iterator<int *>(num + 3), 
                              input_iterator<int *>(num),     input_iterator<int *>(num + 1)));
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num + 2), input_iterator<int *>(num + 3),
                              input_iterator<int *>(num),     input_iterator<int *>(num + 1),
                              eq<int> ));
                              
//  Identical long sequences are equal. 
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz)));
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              eq<int> ));
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              never_eq<int> ));
    BOOST_CHECK ( ba::equal ( input_iterator<int *>(num),             input_iterator<int *>(num + sz),
                              random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz),
                              eq<int> ));

//  different sequences are different
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num + 1), input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz)));
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num + 1), input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              eq<int> ));
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz - 1)));
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz - 1),
                              eq<int> ));

//  When there's a cheap check, bail early
    comparison_count = 0;
    BOOST_CHECK (!ba::equal ( random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz),
                              random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz - 1),
                              counting_equals<int> ));
    BOOST_CHECK ( comparison_count == 0 );
//  And when there's not, we can't
    comparison_count = 0;
    BOOST_CHECK (!ba::equal ( input_iterator<int *>(num),     input_iterator<int *>(num + sz),
                              input_iterator<int *>(num),     input_iterator<int *>(num + sz - 1),
                              counting_equals<int> ));
    BOOST_CHECK ( comparison_count > 0 );
    
}


BOOST_AUTO_TEST_CASE( test_main )
{
  test_equal ();
}
