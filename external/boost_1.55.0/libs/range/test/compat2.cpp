// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/config.hpp>

enum Container {};
enum String {};

template< typename T >
struct range_iterator;

template<>
struct range_iterator<Container>
{
    template< typename C >
    struct pts
    {
        typedef BOOST_DEDUCED_TYPENAME C::iterator type;
    };
};

template<>
struct range_iterator<String>
{
    template< typename C >
    struct pts
    {
        typedef C type;
    };
};

template< typename C >
class iterator_of
{
public:
    typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::BOOST_NESTED_TEMPLATE pts<C>::type type; 
};

#include <vector>

void compat1()
{
    std::vector<int> v;
    iterator_of< std::vector<int> >::type i = v.begin();
}

#include <boost/test/included/unit_test.hpp> 

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add( BOOST_TEST_CASE( &compat1 ) );

    return test;
}






