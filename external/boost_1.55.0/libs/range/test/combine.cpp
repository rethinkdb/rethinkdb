// Boost.Range library
//
//  Copyright Thorsten Ottosen 2006. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#include <boost/range/combine.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <vector>


struct add
{
    template< class T >
    int operator()( const T& tuple ) const
    {
        return boost::get<0>(tuple) + boost::get<1>(tuple);
    }
};

template< class CombinedRng >
void apply( const CombinedRng& r )
{
    std::vector<int> v;
    typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<const CombinedRng>::type iterator_t;

    iterator_t e = boost::end(r);
    for (iterator_t i = boost::begin(r); i != e; ++i)
    {
    }
}

void test_combine()
{
    std::vector<int> v1, v2, v3;
    for (int i = 1; i <= 4; ++i)
    {
        v1.push_back(i);
        v2.push_back(i);
    }

    int i1, i2;
    BOOST_FOREACH( boost::tie( i1, i2 ), boost::combine(v1,v2) )
    {
        v3.push_back( i1 + i2 );
    }

    BOOST_CHECK_EQUAL( v3.size(), v1.size() );
}



using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add( BOOST_TEST_CASE( &test_combine ) );

    return test;
}


