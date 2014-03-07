/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <iostream>

#include <boost/config.hpp>
#include <boost/algorithm/cxx11/is_permutation.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <list>

#include "iterator_test.hpp"

template <typename T>
bool eq ( const T& a, const T& b ) { return a == b; }

template <typename T>
bool never_eq ( const T&, const T& ) { return false; }

namespace ba = boost::algorithm;

void test_sequence1 () {
    int num[] = { 1, 1, 2, 3, 5 };
    const int sz = sizeof (num)/sizeof(num[0]);

//  Empty sequences
    BOOST_CHECK (
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num), 
            forward_iterator<int *>(num)));
    BOOST_CHECK (
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num), 
            forward_iterator<int *>(num),     forward_iterator<int *>(num)));
    BOOST_CHECK (
        ba::is_permutation (
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num), 
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num)));
    BOOST_CHECK (
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num), 
            forward_iterator<int *>(num), 
            never_eq<int> ));       // Since the sequences are empty, the pred is never called
            
//  Empty vs. non-empty
    BOOST_CHECK ( !
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num), 
            forward_iterator<int *>(num),     forward_iterator<int *>(num + 1)));

    BOOST_CHECK ( !
        ba::is_permutation ( 
            forward_iterator<int *>(num + 1), forward_iterator<int *>(num + 2), 
            forward_iterator<int *>(num),     forward_iterator<int *>(num)));
                    
    BOOST_CHECK ( !
        ba::is_permutation (
            random_access_iterator<int *>(num + 1), random_access_iterator<int *>(num + 2), 
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num)));

    BOOST_CHECK ( !
        ba::is_permutation (
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num), 
            random_access_iterator<int *>(num + 1), random_access_iterator<int *>(num + 2)));

//  Something should be a permutation of itself
    BOOST_CHECK (
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num + sz), 
            forward_iterator<int *>(num)));
    BOOST_CHECK (
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num + sz), 
            forward_iterator<int *>(num), eq<int> ));
    BOOST_CHECK (
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num + sz), 
            forward_iterator<int *>(num),     forward_iterator<int *>(num + sz )));
    BOOST_CHECK (
        ba::is_permutation (
            forward_iterator<int *>(num),     forward_iterator<int *>(num + sz), 
            forward_iterator<int *>(num),     forward_iterator<int *>(num + sz ),
            eq<int> ));
            
    BOOST_CHECK (
        ba::is_permutation (
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz), 
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz)));
    BOOST_CHECK (
        ba::is_permutation (
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz), 
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz),
            eq<int> ));
    BOOST_CHECK (
        ba::is_permutation (
            random_access_iterator<int *>(num),     random_access_iterator<int *>(num + sz), 
            forward_iterator<int *>(num),           forward_iterator<int *>(num + sz),
            eq<int> ));
    

    std::vector<int> v, v1;
    
    v.clear ();
    for ( std::size_t i = 5; i < 15; ++i )
        v.push_back ( i );
    v1 = v;
    BOOST_CHECK ( ba::is_permutation ( v.begin (), v.end (), v.begin ()));  // better be a permutation of itself!
    BOOST_CHECK ( ba::is_permutation ( v.begin (), v.end (), v1.begin ()));    

//  With bidirectional iterators.
    std::list<int> l;
    std::copy ( v.begin (), v.end (), std::back_inserter ( l ));
    BOOST_CHECK ( ba::is_permutation ( l.begin (), l.end (), l.begin ()));  // better be a permutation of itself!
    BOOST_CHECK ( ba::is_permutation ( l.begin (), l.end (), v1.begin ()));
    for ( std::size_t i = 0; i < l.size (); ++i ) {
        l.push_back ( *l.begin ()); l.pop_front (); // rotation
        BOOST_CHECK ( ba::is_permutation ( l.begin (), l.end (), v1.begin ()));
        }   
    }


BOOST_AUTO_TEST_CASE( test_main )
{
  test_sequence1 ();
}
