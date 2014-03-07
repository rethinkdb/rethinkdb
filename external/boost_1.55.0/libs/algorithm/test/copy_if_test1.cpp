/* 
   Copyright (c) Marshall Clow 2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <boost/config.hpp>
#include <boost/algorithm/cxx11/copy_if.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <string>
#include <iostream>
#include <vector>
#include <list>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/cxx11/none_of.hpp>

namespace ba = boost::algorithm;
// namespace ba = boost;

bool is_true  ( int v ) { return true; }
bool is_false ( int v ) { return false; }
bool is_even  ( int v ) { return v % 2 == 0; }
bool is_odd   ( int v ) { return v % 2 == 1; }

template <typename Container>
void test_copy_if ( Container const &c ) {

    typedef typename Container::value_type value_type;
    std::vector<value_type> v;
    
//  None of the elements
    v.clear ();
    ba::copy_if ( c.begin (), c.end (), back_inserter ( v ), is_false);
    BOOST_CHECK ( v.size () == 0 );

    v.clear ();
    ba::copy_if ( c, back_inserter ( v ), is_false);
    BOOST_CHECK ( v.size () == 0 );

//  All the elements
    v.clear ();
    ba::copy_if ( c.begin (), c.end (), back_inserter ( v ), is_true);
    BOOST_CHECK ( v.size () == c.size ());
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

    v.clear ();
    ba::copy_if ( c, back_inserter ( v ), is_true);
    BOOST_CHECK ( v.size () == c.size ());
    BOOST_CHECK ( v.size () == c.size ());
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

//  Some of the elements
    v.clear ();
    ba::copy_if ( c.begin (), c.end (), back_inserter ( v ), is_even );
    BOOST_CHECK ( v.size () == (size_t) std::count_if ( c.begin (), c.end (), is_even ));
    BOOST_CHECK ( ba::all_of ( v.begin (), v.end (), is_even ));

    v.clear ();
    ba::copy_if ( c, back_inserter ( v ), is_even );
    BOOST_CHECK ( v.size () == (size_t) std::count_if ( c.begin (), c.end (), is_even ));
    BOOST_CHECK ( ba::all_of ( v.begin (), v.end (), is_even ));
    }


template <typename Container>
void test_copy_while ( Container const &c ) {

    typedef typename Container::value_type value_type;
    typename Container::const_iterator it;
    std::vector<value_type> v;
    
//  None of the elements
    v.clear ();
    ba::copy_while ( c.begin (), c.end (), back_inserter ( v ), is_false);
    BOOST_CHECK ( v.size () == 0 );
    
    v.clear ();
    ba::copy_while ( c, back_inserter ( v ), is_false);
    BOOST_CHECK ( v.size () == 0 );

//  All the elements
    v.clear ();
    ba::copy_while ( c.begin (), c.end (), back_inserter ( v ), is_true);
    BOOST_CHECK ( v.size () == c.size ());
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

    v.clear ();
    ba::copy_while ( c, back_inserter ( v ), is_true);
    BOOST_CHECK ( v.size () == c.size ());
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

//  Some of the elements
    v.clear ();
    it = ba::copy_while ( c.begin (), c.end (), back_inserter ( v ), is_even ).first;
    BOOST_CHECK ( v.size () == (size_t) std::distance ( c.begin (), it ));
    BOOST_CHECK ( it == c.end () || !is_even ( *it ));
    BOOST_CHECK ( ba::all_of ( v.begin (), v.end (), is_even ));
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

    v.clear ();
    it = ba::copy_while ( c, back_inserter ( v ), is_even ).first;
    BOOST_CHECK ( v.size () == (size_t) std::distance ( c.begin (), it ));
    BOOST_CHECK ( it == c.end () || !is_even ( *it ));
    BOOST_CHECK ( ba::all_of ( v.begin (), v.end (), is_even ));
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));
    }

template <typename Container>
void test_copy_until ( Container const &c ) {

    typedef typename Container::value_type value_type;
    typename Container::const_iterator it;
    std::vector<value_type> v;
    
//  None of the elements
    v.clear ();
    ba::copy_until ( c.begin (), c.end (), back_inserter ( v ), is_true);
    BOOST_CHECK ( v.size () == 0 );

    v.clear ();
    ba::copy_until ( c, back_inserter ( v ), is_true);
    BOOST_CHECK ( v.size () == 0 );

//  All the elements
    v.clear ();
    ba::copy_until ( c.begin (), c.end (), back_inserter ( v ), is_false);
    BOOST_CHECK ( v.size () == c.size ());
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

    v.clear ();
    ba::copy_until ( c, back_inserter ( v ), is_false);
    BOOST_CHECK ( v.size () == c.size ());
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

//  Some of the elements
    v.clear ();
    it = ba::copy_until ( c.begin (), c.end (), back_inserter ( v ), is_even ).first;
    BOOST_CHECK ( v.size () == (size_t) std::distance ( c.begin (), it ));
    BOOST_CHECK ( it == c.end () || is_even ( *it ));
    BOOST_CHECK ( ba::none_of ( v.begin (), v.end (), is_even ));
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));

    v.clear ();
    it = ba::copy_until ( c, back_inserter ( v ), is_even ).first;
    BOOST_CHECK ( v.size () == (size_t) std::distance ( c.begin (), it ));
    BOOST_CHECK ( it == c.end () || is_even ( *it ));
    BOOST_CHECK ( ba::none_of ( v.begin (), v.end (), is_even ));
    BOOST_CHECK ( std::equal ( v.begin (), v.end (), c.begin ()));
    }
    
void test_sequence1 () {
    std::vector<int> v;
    for ( int i = 5; i < 15; ++i )
        v.push_back ( i );
    test_copy_if ( v );
    test_copy_while ( v );
    test_copy_until ( v );
    
    std::list<int> l;
    for ( int i = 25; i > 15; --i )
        l.push_back ( i );
    test_copy_if ( l );
    test_copy_while ( l );
    test_copy_until ( l );
    }


BOOST_AUTO_TEST_CASE( test_main )
{
  test_sequence1 ();
}
