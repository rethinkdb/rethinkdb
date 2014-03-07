/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <iostream>

#include <boost/config.hpp>
#include <boost/algorithm/cxx11/is_partitioned.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <list>

namespace ba = boost::algorithm;
// namespace ba = boost;

template <typename T>
struct less_than {
public:
    less_than ( T foo ) : val ( foo ) {}
    less_than ( const less_than &rhs ) : val ( rhs.val ) {}

    bool operator () ( const T &v ) const { return v < val; }
private:
    less_than ();
    less_than operator = ( const less_than &rhs );
    T val;
    };


void test_sequence1 () {
    std::vector<int> v;
    
    v.clear ();
    for ( int i = 5; i < 15; ++i )
        v.push_back ( i );
    BOOST_CHECK ( ba::is_partitioned ( v, less_than<int>(3)));      // no elements
    BOOST_CHECK ( ba::is_partitioned ( v, less_than<int>(6)));      // only the first element
    BOOST_CHECK ( ba::is_partitioned ( v, less_than<int>(10))); // in the middle somewhere
    BOOST_CHECK ( ba::is_partitioned ( v, less_than<int>(99))); // all elements satisfy 

//  With bidirectional iterators.
    std::list<int> l;
    for ( int i = 5; i < 15; ++i )
        l.push_back ( i );
    BOOST_CHECK ( ba::is_partitioned ( l.begin (), l.end (), less_than<int>(3)));       // no elements
    BOOST_CHECK ( ba::is_partitioned ( l.begin (), l.end (), less_than<int>(6)));       // only the first element
    BOOST_CHECK ( ba::is_partitioned ( l.begin (), l.end (), less_than<int>(10)));  // in the middle somewhere
    BOOST_CHECK ( ba::is_partitioned ( l.begin (), l.end (), less_than<int>(99)));      // all elements satisfy 
    }


BOOST_AUTO_TEST_CASE( test_main )
{
  test_sequence1 ();
}
