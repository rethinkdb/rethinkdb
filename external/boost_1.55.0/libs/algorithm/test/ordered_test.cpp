//  Copyright (c) 2010 Nuovation System Designs, LLC
//    Grant Erickson <gerickson@nuovations.com>
//
//  Reworked by Marshall Clow; August 2010
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/ for latest version.

#include <algorithm>
#include <iostream>

#include <boost/algorithm/cxx11/is_sorted.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost;

/* Preprocessor Defines */

#define elementsof(v)   (sizeof (v) / sizeof (v[0]))
#define a_begin(v)      (&v[0])
#define a_end(v)        (v + elementsof (v))
#define a_range(v)      v
#define b_e(v)          a_begin(v),a_end(v)

namespace ba = boost::algorithm;

static void
test_ordered(void)
{
    const int strictlyIncreasingValues[] = { 1, 2, 3, 4, 5 };
    const int randomValues[] = { 3, 6, 1, 2, 7 };
    const int constantValues[] = { 1, 2, 2, 2, 5 };
          int nonConstantArray[] = { 1, 2, 2, 2, 5 };
    const int inOrderUntilTheEnd [] = { 0, 1, 2, 3, 4, 5, 6, 7, 6 };

//  Begin/end checks
    BOOST_CHECK (  ba::is_sorted (b_e(strictlyIncreasingValues)));
    BOOST_CHECK ( !ba::is_sorted (b_e(randomValues)));
    BOOST_CHECK (  ba::is_sorted (b_e(strictlyIncreasingValues), std::less<int>()));
    BOOST_CHECK ( !ba::is_sorted (b_e(strictlyIncreasingValues), std::greater<int>()));

//  Range checks
    BOOST_CHECK (  ba::is_sorted (a_range(strictlyIncreasingValues)));
    BOOST_CHECK ( !ba::is_sorted (a_range(randomValues)));
    BOOST_CHECK (  ba::is_sorted (a_range(strictlyIncreasingValues), std::less<int>()));
    BOOST_CHECK ( !ba::is_sorted (a_range(strictlyIncreasingValues), std::greater<int>()));

    BOOST_CHECK (  ba::is_sorted_until ( b_e(strictlyIncreasingValues))                       ==      a_end(strictlyIncreasingValues));
    BOOST_CHECK (  ba::is_sorted_until ( b_e(strictlyIncreasingValues),     std::less<int>()) ==      a_end(strictlyIncreasingValues));
    BOOST_CHECK (  ba::is_sorted_until ( a_range(strictlyIncreasingValues))                   == boost::end(strictlyIncreasingValues));
    BOOST_CHECK (  ba::is_sorted_until ( a_range(strictlyIncreasingValues), std::less<int>()) == boost::end(strictlyIncreasingValues));

//  Check for const and non-const arrays
    BOOST_CHECK ( ba::is_sorted_until ( b_e(constantValues),       std::less<int>()) ==      a_end(constantValues));
    BOOST_CHECK ( ba::is_sorted_until ( a_range(constantValues),   std::less<int>()) == boost::end(constantValues));
    BOOST_CHECK ( ba::is_sorted_until ( b_e(nonConstantArray),     std::less<int>()) ==      a_end(nonConstantArray));
    BOOST_CHECK ( ba::is_sorted_until ( a_range(nonConstantArray), std::less<int>()) == boost::end(nonConstantArray));

    BOOST_CHECK ( ba::is_sorted_until ( b_e(randomValues),     std::less<int>()) == &randomValues[2] );
    BOOST_CHECK ( ba::is_sorted_until ( b_e(randomValues))                       == &randomValues[2] );
    BOOST_CHECK ( ba::is_sorted_until ( a_range(randomValues), std::less<int>()) == &randomValues[2] );
    BOOST_CHECK ( ba::is_sorted_until ( a_range(randomValues))                   == &randomValues[2] );

    BOOST_CHECK ( ba::is_sorted_until ( a_range(inOrderUntilTheEnd), std::less<int>()) == &inOrderUntilTheEnd[8] );
    BOOST_CHECK ( ba::is_sorted_until ( a_range(inOrderUntilTheEnd))                   == &inOrderUntilTheEnd[8] );

//  For zero and one element collections, the comparison predicate should never be called
    BOOST_CHECK ( ba::is_sorted_until ( a_begin(randomValues), a_begin(randomValues),     std::equal_to<int>()) == a_begin(randomValues));
    BOOST_CHECK ( ba::is_sorted_until ( a_begin(randomValues), a_begin(randomValues))                           == a_begin(randomValues));
    BOOST_CHECK ( ba::is_sorted_until ( a_begin(randomValues), a_begin(randomValues) + 1, std::equal_to<int>()) == a_begin(randomValues) + 1);
    BOOST_CHECK ( ba::is_sorted_until ( a_begin(randomValues), a_begin(randomValues) + 1 )                      == a_begin(randomValues) + 1);
}


static void
test_increasing_decreasing(void)
{
    const int strictlyIncreasingValues[] = { 1, 2, 3, 4, 5 };
    const int strictlyDecreasingValues[] = { 9, 8, 7, 6, 5 };
    const int increasingValues[] = { 1, 2, 2, 2, 5 };
    const int decreasingValues[] = { 9, 7, 7, 7, 5 };
    const int randomValues[] = { 3, 6, 1, 2, 7 };
    const int constantValues[] = { 7, 7, 7, 7, 7 };

    // Test a strictly increasing sequence
    BOOST_CHECK (  ba::is_strictly_increasing (b_e(strictlyIncreasingValues)));
    BOOST_CHECK (  ba::is_increasing          (b_e(strictlyIncreasingValues)));
    BOOST_CHECK ( !ba::is_strictly_decreasing (b_e(strictlyIncreasingValues)));
    BOOST_CHECK ( !ba::is_decreasing          (b_e(strictlyIncreasingValues)));

    BOOST_CHECK (  ba::is_strictly_increasing (a_range(strictlyIncreasingValues)));
    BOOST_CHECK (  ba::is_increasing          (a_range(strictlyIncreasingValues)));
    BOOST_CHECK ( !ba::is_strictly_decreasing (a_range(strictlyIncreasingValues)));
    BOOST_CHECK ( !ba::is_decreasing          (a_range(strictlyIncreasingValues)));

    // Test a strictly decreasing sequence
    BOOST_CHECK ( !ba::is_strictly_increasing (b_e(strictlyDecreasingValues)));
    BOOST_CHECK ( !ba::is_increasing          (b_e(strictlyDecreasingValues)));
    BOOST_CHECK (  ba::is_strictly_decreasing (b_e(strictlyDecreasingValues)));
    BOOST_CHECK (  ba::is_decreasing          (b_e(strictlyDecreasingValues)));

    // Test an increasing sequence
    BOOST_CHECK ( !ba::is_strictly_increasing (b_e(increasingValues)));
    BOOST_CHECK (  ba::is_increasing          (b_e(increasingValues)));
    BOOST_CHECK ( !ba::is_strictly_decreasing (b_e(increasingValues)));
    BOOST_CHECK ( !ba::is_decreasing          (b_e(increasingValues)));

    // Test a decreasing sequence
    BOOST_CHECK ( !ba::is_strictly_increasing (b_e(decreasingValues)));
    BOOST_CHECK ( !ba::is_increasing          (b_e(decreasingValues)));
    BOOST_CHECK ( !ba::is_strictly_decreasing (b_e(decreasingValues)));
    BOOST_CHECK (  ba::is_decreasing          (b_e(decreasingValues)));

    // Test a random sequence
    BOOST_CHECK ( !ba::is_strictly_increasing (b_e(randomValues)));
    BOOST_CHECK ( !ba::is_increasing          (b_e(randomValues)));
    BOOST_CHECK ( !ba::is_strictly_decreasing (b_e(randomValues)));
    BOOST_CHECK ( !ba::is_decreasing          (b_e(randomValues)));

    // Test a constant sequence
    BOOST_CHECK ( !ba::is_strictly_increasing (b_e(constantValues)));
    BOOST_CHECK (  ba::is_increasing          (b_e(constantValues)));
    BOOST_CHECK ( !ba::is_strictly_decreasing (b_e(constantValues)));
    BOOST_CHECK (  ba::is_decreasing          (b_e(constantValues)));
    
    // Test an empty sequence
    BOOST_CHECK (  ba::is_strictly_increasing (strictlyIncreasingValues, strictlyIncreasingValues));
    BOOST_CHECK (  ba::is_increasing          (strictlyIncreasingValues, strictlyIncreasingValues));
    BOOST_CHECK (  ba::is_strictly_decreasing (strictlyIncreasingValues, strictlyIncreasingValues));
    BOOST_CHECK (  ba::is_decreasing          (strictlyIncreasingValues, strictlyIncreasingValues));
    
    // Test a one-element sequence
    BOOST_CHECK (  ba::is_strictly_increasing (strictlyIncreasingValues, strictlyIncreasingValues+1));
    BOOST_CHECK (  ba::is_increasing          (strictlyIncreasingValues, strictlyIncreasingValues+1));
    BOOST_CHECK (  ba::is_strictly_decreasing (strictlyIncreasingValues, strictlyIncreasingValues+1));
    BOOST_CHECK (  ba::is_decreasing          (strictlyIncreasingValues, strictlyIncreasingValues+1));

    // Test a two-element sequence
    BOOST_CHECK (  ba::is_strictly_increasing (strictlyIncreasingValues, strictlyIncreasingValues+2));
    BOOST_CHECK (  ba::is_increasing          (strictlyIncreasingValues, strictlyIncreasingValues+2));
    BOOST_CHECK ( !ba::is_strictly_decreasing (strictlyIncreasingValues, strictlyIncreasingValues+2));
    BOOST_CHECK ( !ba::is_decreasing          (strictlyIncreasingValues, strictlyIncreasingValues+2));
    
}

BOOST_AUTO_TEST_CASE( test_main )
{
    test_ordered ();
    test_increasing_decreasing ();
}
