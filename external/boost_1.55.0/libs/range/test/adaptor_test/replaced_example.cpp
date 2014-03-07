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
//[replaced_example
#include <boost/range/adaptor/replaced.hpp>
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
void replaced_example_test()
//->
//=int main(int argc, const char* argv[])
{
    using namespace boost::adaptors;
    using namespace boost::assign;

    std::vector<int> input;
    input += 1,2,3,2,5,2,7,2,9;
    
    boost::copy(
        input | replaced(2, 10),
        std::ostream_iterator<int>(std::cout, ","));

//=    return 0;
//=}
//]
    std::vector<int> reference;
    reference += 1,10,3,10,5,10,7,10,9;

    std::vector<int> test;
    boost::push_back(test, input | replaced(2, 10));

    BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
        test.begin(), test.end() );
}
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.replaced_example" );

    test->add( BOOST_TEST_CASE( &replaced_example_test ) );

    return test;
}
