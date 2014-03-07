//  (C) Copyright Jesse Williamson 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <vector>

#include <boost/config.hpp>
#include <boost/algorithm/clamp.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

namespace ba = boost::algorithm;

bool intGreater    ( int lhs, int rhs )       { return lhs > rhs; }
bool doubleGreater ( double lhs, double rhs ) { return lhs > rhs; }

class custom {
public:
    custom ( int x )             : v(x)     {}
    custom ( const custom &rhs ) : v(rhs.v) {}
    ~custom () {}
    custom & operator = ( const custom &rhs ) { v = rhs.v; return *this; }
    
    bool operator <  ( const custom &rhs ) const { return v < rhs.v; }
    bool operator == ( const custom &rhs ) const { return v == rhs.v; }     // need this for the test
    
    std::ostream & print ( std::ostream &os ) const { return os << v; }
    
    int v;
    };

std::ostream & operator << ( std::ostream & os, const custom &x ) { return x.print ( os ); }

bool customLess ( const custom &lhs, const custom &rhs ) { return lhs.v < rhs.v; }

void test_ints()
{

//  Inside the range, equal to the endpoints, and outside the endpoints.
    BOOST_CHECK_EQUAL (  3, ba::clamp (  3, 1, 10 ));
    BOOST_CHECK_EQUAL (  1, ba::clamp (  1, 1, 10 ));
    BOOST_CHECK_EQUAL (  1, ba::clamp (  0, 1, 10 ));
    BOOST_CHECK_EQUAL ( 10, ba::clamp ( 10, 1, 10 ));
    BOOST_CHECK_EQUAL ( 10, ba::clamp ( 11, 1, 10 ));
    
    BOOST_CHECK_EQUAL (  3, ba::clamp (  3, 10, 1, intGreater ));
    BOOST_CHECK_EQUAL (  1, ba::clamp (  1, 10, 1, intGreater ));
    BOOST_CHECK_EQUAL (  1, ba::clamp (  0, 10, 1, intGreater ));
    BOOST_CHECK_EQUAL ( 10, ba::clamp ( 10, 10, 1, intGreater ));
    BOOST_CHECK_EQUAL ( 10, ba::clamp ( 11, 10, 1, intGreater ));

//  Negative numbers
    BOOST_CHECK_EQUAL (  -3, ba::clamp (  -3, -10, -1 ));
    BOOST_CHECK_EQUAL (  -1, ba::clamp (  -1, -10, -1 ));
    BOOST_CHECK_EQUAL (  -1, ba::clamp (   0, -10, -1 ));
    BOOST_CHECK_EQUAL ( -10, ba::clamp ( -10, -10, -1 ));
    BOOST_CHECK_EQUAL ( -10, ba::clamp ( -11, -10, -1 ));

//  Mixed positive and negative numbers
    BOOST_CHECK_EQUAL (   5, ba::clamp (   5, -10, 10 ));
    BOOST_CHECK_EQUAL ( -10, ba::clamp ( -10, -10, 10 ));
    BOOST_CHECK_EQUAL ( -10, ba::clamp ( -15, -10, 10 ));
    BOOST_CHECK_EQUAL (  10, ba::clamp (  10, -10, 10 ));
    BOOST_CHECK_EQUAL (  10, ba::clamp (  15, -10, 10 ));

//  Unsigned 
    BOOST_CHECK_EQUAL (  5U, ba::clamp (  5U, 1U, 10U ));
    BOOST_CHECK_EQUAL (  1U, ba::clamp (  1U, 1U, 10U ));
    BOOST_CHECK_EQUAL (  1U, ba::clamp (  0U, 1U, 10U ));
    BOOST_CHECK_EQUAL ( 10U, ba::clamp ( 10U, 1U, 10U ));
    BOOST_CHECK_EQUAL ( 10U, ba::clamp ( 15U, 1U, 10U ));
    
//  Mixed (1)
    BOOST_CHECK_EQUAL (  5U, ba::clamp (  5U, 1,  10 ));
    BOOST_CHECK_EQUAL (  1U, ba::clamp (  1U, 1,  10 ));
    BOOST_CHECK_EQUAL (  1U, ba::clamp (  0U, 1,  10 ));
    BOOST_CHECK_EQUAL ( 10U, ba::clamp ( 10U, 1,  10 ));
    BOOST_CHECK_EQUAL ( 10U, ba::clamp ( 15U, 1,  10 ));
    
//  Mixed (3)
    BOOST_CHECK_EQUAL (  5U, ba::clamp (  5U, 1,  10. ));
    BOOST_CHECK_EQUAL (  1U, ba::clamp (  1U, 1,  10. ));
    BOOST_CHECK_EQUAL (  1U, ba::clamp (  0U, 1,  10. ));
    BOOST_CHECK_EQUAL ( 10U, ba::clamp ( 10U, 1,  10. ));
    BOOST_CHECK_EQUAL ( 10U, ba::clamp ( 15U, 1,  10. ));
    
    short foo = 50;
    BOOST_CHECK_EQUAL ( 56,     ba::clamp ( foo, 56.9, 129 ));
    BOOST_CHECK_EQUAL ( 24910,  ba::clamp ( foo, 12345678, 123456999 ));
    }


void test_floats()
{

//  Inside the range, equal to the endpoints, and outside the endpoints.
    BOOST_CHECK_EQUAL (  3.0, ba::clamp (  3.0, 1.0, 10.0 ));
    BOOST_CHECK_EQUAL (  1.0, ba::clamp (  1.0, 1.0, 10.0 ));
    BOOST_CHECK_EQUAL (  1.0, ba::clamp (  0.0, 1.0, 10.0 ));
    BOOST_CHECK_EQUAL ( 10.0, ba::clamp ( 10.0, 1.0, 10.0 ));
    BOOST_CHECK_EQUAL ( 10.0, ba::clamp ( 11.0, 1.0, 10.0 ));
    
    BOOST_CHECK_EQUAL (  3.0, ba::clamp (  3.0, 10.0, 1.0, doubleGreater ));
    BOOST_CHECK_EQUAL (  1.0, ba::clamp (  1.0, 10.0, 1.0, doubleGreater ));
    BOOST_CHECK_EQUAL (  1.0, ba::clamp (  0.0, 10.0, 1.0, doubleGreater ));
    BOOST_CHECK_EQUAL ( 10.0, ba::clamp ( 10.0, 10.0, 1.0, doubleGreater ));
    BOOST_CHECK_EQUAL ( 10.0, ba::clamp ( 11.0, 10.0, 1.0, doubleGreater ));

//  Negative numbers
    BOOST_CHECK_EQUAL (  -3.f, ba::clamp (  -3.f, -10.f, -1.f ));
    BOOST_CHECK_EQUAL (  -1.f, ba::clamp (  -1.f, -10.f, -1.f ));
    BOOST_CHECK_EQUAL (  -1.f, ba::clamp (   0.f, -10.f, -1.f ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -10.f, -10.f, -1.f ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -11.f, -10.f, -1.f ));

//  Mixed positive and negative numbers
    BOOST_CHECK_EQUAL (   5.f, ba::clamp (   5.f, -10.f, 10.f ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -10.f, -10.f, 10.f ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -15.f, -10.f, 10.f ));
    BOOST_CHECK_EQUAL (  10.f, ba::clamp (  10.f, -10.f, 10.f ));
    BOOST_CHECK_EQUAL (  10.f, ba::clamp (  15.f, -10.f, 10.f ));

//  Mixed (1)
    BOOST_CHECK_EQUAL (   5.f, ba::clamp (   5.f, -10., 10. ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -10.f, -10., 10. ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -15.f, -10., 10. ));
    BOOST_CHECK_EQUAL (  10.f, ba::clamp (  10.f, -10., 10. ));
    BOOST_CHECK_EQUAL (  10.f, ba::clamp (  15.f, -10., 10. ));

//  Mixed (2)
    BOOST_CHECK_EQUAL (   5.f, ba::clamp (   5.f, -10, 10 ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -10.f, -10, 10 ));
    BOOST_CHECK_EQUAL ( -10.f, ba::clamp ( -15.f, -10, 10 ));
    BOOST_CHECK_EQUAL (  10.f, ba::clamp (  10.f, -10, 10 ));
    BOOST_CHECK_EQUAL (  10.f, ba::clamp (  15.f, -10, 10 ));
}

void test_custom()
{

//  Inside the range, equal to the endpoints, and outside the endpoints.
    BOOST_CHECK_EQUAL ( custom( 3), ba::clamp ( custom( 3), custom(1), custom(10)));
    BOOST_CHECK_EQUAL ( custom( 1), ba::clamp ( custom( 1), custom(1), custom(10)));
    BOOST_CHECK_EQUAL ( custom( 1), ba::clamp ( custom( 0), custom(1), custom(10)));
    BOOST_CHECK_EQUAL ( custom(10), ba::clamp ( custom(10), custom(1), custom(10)));
    BOOST_CHECK_EQUAL ( custom(10), ba::clamp ( custom(11), custom(1), custom(10)));

    BOOST_CHECK_EQUAL ( custom( 3), ba::clamp ( custom( 3), custom(1), custom(10), customLess ));
    BOOST_CHECK_EQUAL ( custom( 1), ba::clamp ( custom( 1), custom(1), custom(10), customLess ));
    BOOST_CHECK_EQUAL ( custom( 1), ba::clamp ( custom( 0), custom(1), custom(10), customLess ));
    BOOST_CHECK_EQUAL ( custom(10), ba::clamp ( custom(10), custom(1), custom(10), customLess ));
    BOOST_CHECK_EQUAL ( custom(10), ba::clamp ( custom(11), custom(1), custom(10), customLess ));

//  Fail!!
//  BOOST_CHECK_EQUAL ( custom(1), ba::clamp ( custom(11), custom(1), custom(10)));
}

#define elementsof(v)   (sizeof (v) / sizeof (v[0]))
#define a_begin(v)      (&v[0])
#define a_end(v)        (v + elementsof (v))
#define a_range(v)      v
#define b_e(v)          a_begin(v),a_end(v)

void test_int_range ()
{
    int inputs []  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 19, 99, 999, -1, -3, -99, 234234 };
    int outputs [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10,  10, -1, -1, -1,  10 };
    std::vector<int> results;
    std::vector<int> in_v;
    
    std::copy ( a_begin(inputs), a_end(inputs), std::back_inserter ( in_v ));
    
    ba::clamp_range ( a_begin(inputs), a_end(inputs), std::back_inserter ( results ), -1, 10 );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();
    ba::clamp_range ( in_v.begin (), in_v.end (), std::back_inserter ( results ), -1, 10 );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();

    ba::clamp_range ( a_begin(inputs), a_end(inputs), std::back_inserter ( results ), 10, -1, intGreater );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();
    ba::clamp_range ( in_v.begin (), in_v.end (), std::back_inserter ( results ), 10, -1, intGreater );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();

    ba::clamp_range ( a_range(inputs), std::back_inserter ( results ), -1, 10 );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();
    ba::clamp_range ( in_v, std::back_inserter ( results ), -1, 10 );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();
        
    ba::clamp_range ( a_range(inputs), std::back_inserter ( results ), 10, -1, intGreater );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();
    ba::clamp_range ( in_v, std::back_inserter ( results ), 10, -1, intGreater );
    BOOST_CHECK ( std::equal ( results.begin(), results.end (), outputs ));
    results.clear ();
    
    int junk[elementsof(inputs)];
    ba::clamp_range ( inputs, junk, 10, -1, intGreater );
    BOOST_CHECK ( std::equal ( b_e(junk), outputs ));
}

BOOST_AUTO_TEST_CASE( test_main )
{
    test_ints ();
    test_floats ();
    test_custom ();
    
    test_int_range ();
//    test_float_range ();
//    test_custom_range ();
}
