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
//[indirected_example
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/shared_ptr.hpp>
#include <iterator>
#include <iostream>
#include <vector>

//<-
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/algorithm_ext/push_back.hpp>

namespace 
{
void indirected_example_test()
//->
//=int main(int argc, const char* argv[])
{
    using namespace boost::adaptors;

    std::vector<boost::shared_ptr<int> > input;

    for (int i = 0; i < 10; ++i)
        input.push_back(boost::shared_ptr<int>(new int(i)));
    
    boost::copy(
        input | indirected,
        std::ostream_iterator<int>(std::cout, ","));

//=    return 0;
//=}
//]
    std::vector<int> reference;
    for (int i = 0; i < 10; ++i)
        reference.push_back(i);

    std::vector<int> test;
    boost::push_back(test, input | indirected);

    BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
        test.begin(), test.end() );
}
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.indirected_example" );

    test->add( BOOST_TEST_CASE( &indirected_example_test ) );

    return test;
}
