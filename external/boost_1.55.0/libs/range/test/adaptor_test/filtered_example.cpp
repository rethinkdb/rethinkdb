// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
//[filtered_example
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <iterator>
#include <iostream>
#include <vector>

//<-
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/algorithm_ext/push_back.hpp>

namespace 
{
//->    
struct is_even
{
    bool operator()( int x ) const { return x % 2 == 0; }
};

//<-
void filtered_example_test()
//->
//=int main(int argc, const char* argv[])
{
    using namespace boost::assign;
    using namespace boost::adaptors;

    std::vector<int> input;
    input += 1,2,3,4,5,6,7,8,9;

    boost::copy(
        input | filtered(is_even()),
        std::ostream_iterator<int>(std::cout, ","));

//=    return 0;
//=}
//]
    std::vector<int> reference;
    reference += 2,4,6,8;

    std::vector<int> test;
    boost::push_back(test, input | filtered(is_even()));

    BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
        test.begin(), test.end() );
}
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.filtered_example" );

    test->add( BOOST_TEST_CASE( &filtered_example_test ) );

    return test;
}
