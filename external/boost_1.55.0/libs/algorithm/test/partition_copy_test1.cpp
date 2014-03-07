/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <iostream>

#include <boost/config.hpp>
#include <boost/algorithm/cxx11/partition_copy.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/cxx11/none_of.hpp>
#include <string>
#include <vector>
#include <list>

namespace ba = boost::algorithm;
// namespace ba = boost;

template <typename Container, typename Predicate>
void test_sequence ( const Container &c, Predicate comp ) {
    std::vector<typename Container::value_type> v1, v2;
    
    v1.clear (); v2.clear ();
    ba::partition_copy ( c.begin (), c.end (), 
                std::back_inserter (v1), std::back_inserter (v2), comp );
//  std::cout << "Sizes(1): " << c.size () << " -> { " << v1.size () << ", " << v2.size () << " }" << std::endl;
    BOOST_CHECK ( v1.size () + v2.size () == c.size ());
    BOOST_CHECK ( ba::all_of  ( v1.begin (), v1.end (), comp ));
    BOOST_CHECK ( ba::none_of ( v2.begin (), v2.end (), comp ));

    v1.clear (); v2.clear ();
    ba::partition_copy ( c, std::back_inserter (v1), std::back_inserter ( v2 ), comp );
//  std::cout << "Sizes(2): " << c.size () << " -> { " << v1.size () << ", " << v2.size () << " }" << std::endl;
    BOOST_CHECK ( v1.size () + v2.size () == c.size ());
    BOOST_CHECK ( ba::all_of  ( v1, comp ));
    BOOST_CHECK ( ba::none_of ( v2, comp ));
    }

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

bool is_even ( int v ) { return v % 2 == 0; }

void test_sequence1 () {
    std::vector<int> v;
    
    v.clear ();
    for ( int i = 5; i < 15; ++i )
        v.push_back ( i );
    test_sequence ( v, less_than<int>(3));      // no elements
    test_sequence ( v, less_than<int>(6));      // only the first element
    test_sequence ( v, less_than<int>(10));
    test_sequence ( v, less_than<int>(99));     // all elements satisfy 

//  With bidirectional iterators.
    std::list<int> l;
    for ( int i = 5; i < 16; ++i )
        l.push_back ( i );
    test_sequence ( l, less_than<int>(3));      // no elements
    test_sequence ( l, less_than<int>(6));      // only the first element
    test_sequence ( l, less_than<int>(10));
    test_sequence ( l, less_than<int>(99));     // all elements satisfy 

    }


BOOST_AUTO_TEST_CASE( test_main )
{
  test_sequence1 ();
}
