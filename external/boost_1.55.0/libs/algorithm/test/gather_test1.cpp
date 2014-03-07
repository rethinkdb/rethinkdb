/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <iostream>

#include <boost/config.hpp>
#include <boost/algorithm/gather.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <list>

#include "iterator_test.hpp"

namespace ba = boost::algorithm;

template <typename Container>
void print ( const char *prompt, const Container &c ) {
    std::cout << prompt << " { ";
    std::copy ( c.begin (), c.end (), std::ostream_iterator<typename Container::value_type>(std::cout, " "));
    std::cout << std::endl;
    }

template <typename Iterator, typename Predicate>
void test_iterators ( Iterator first, Iterator last, Predicate comp, std::size_t offset ) {
//  Create the pivot point
    Iterator off = first;
    std::advance(off, offset);
        
//  Gather the elements
    std::pair<Iterator, Iterator> res = ba::gather ( first, last, off, comp );

//  We should now have three sequences, any of which may be empty:
//      * [begin .. result.first)         - items that do not satisfy the predicate
//      * [result.first .. result.second) - items that do satisfy the predicate
//      * [result.second .. end)          - items that do not satisfy the predicate
    Iterator iter = first;
    for ( ; iter != res.first; ++iter )
        BOOST_CHECK ( !comp ( *iter ));
    for ( ; iter != res.second; ++iter)
        BOOST_CHECK ( comp ( *iter ));
    for ( ; iter != last; ++iter )
        BOOST_CHECK ( !comp ( *iter ));
    }

template <typename Container, typename Predicate>
void test_iterator_types ( const Container &c, Predicate comp, std::size_t offset ) {
    typedef std::vector<typename Container::value_type> vec;

    typedef bidirectional_iterator<typename vec::iterator> BDI;
    typedef random_access_iterator<typename vec::iterator> RAI;
    
    vec v;
    v.assign ( c.begin (), c.end ());
    test_iterators ( BDI ( v.begin ()), BDI ( v.end ()), comp, offset );
    v.assign ( c.begin (), c.end ());
    test_iterators ( RAI ( v.begin ()), RAI ( v.end ()), comp, offset );
    }


template <typename T>
struct less_than {
public:
//    typedef T argument_type;
//    typedef bool result_type;

    less_than ( T foo ) : val ( foo ) {}
    less_than ( const less_than &rhs ) : val ( rhs.val ) {}

    bool operator () ( const T &v ) const { return v < val; }
private:
    less_than ();
    less_than operator = ( const less_than &rhs );
    T val;
    };

bool is_even ( int i ) { return i % 2 == 0; }
bool is_ten  ( int i ) { return i == 10; }

void test_sequence1 () {
    std::vector<int> v;
    
    for ( int i = 5; i < 15; ++i )
        v.push_back ( i );
    test_iterator_types ( v, less_than<int>(10),  0 );                  // at beginning
    test_iterator_types ( v, less_than<int>(10),  5 );
    test_iterator_types ( v, less_than<int>(10), v.size () - 1 );       // at end

    test_iterator_types ( v, is_even, 0 );
    test_iterator_types ( v, is_even, 5 );
    test_iterator_types ( v, is_even, v.size () - 1 );

//  Exactly one element in the sequence matches
    test_iterator_types ( v, is_ten, 0 );
    test_iterator_types ( v, is_ten, 5 );
    test_iterator_types ( v, is_ten, v.size () - 1 );

//  Everything in the sequence matches
    test_iterator_types ( v, less_than<int>(99),  0 );
    test_iterator_types ( v, less_than<int>(99),  5 );
    test_iterator_types ( v, less_than<int>(99), v.size () - 1 );
    
//  Nothing in the sequence matches
    test_iterator_types ( v, less_than<int>(0),  0 );
    test_iterator_types ( v, less_than<int>(0),  5 );
    test_iterator_types ( v, less_than<int>(0), v.size () - 1 );
    
//  All the elements in the sequence are the same
    v.clear ();
    for ( int i = 0; i < 11; ++i )
        v.push_back ( 10 );
    
//  Everything in the sequence matches
    test_iterator_types ( v, is_ten, 0 );
    test_iterator_types ( v, is_ten, 5 );
    test_iterator_types ( v, is_ten, v.size () - 1 );

//  Nothing in the sequence matches
    test_iterator_types ( v, less_than<int>(5),  0 );
    test_iterator_types ( v, less_than<int>(5),  5 );
    test_iterator_types ( v, less_than<int>(5), v.size () - 1 );

    }


BOOST_AUTO_TEST_CASE( test_main )
{
  test_sequence1 ();
}
