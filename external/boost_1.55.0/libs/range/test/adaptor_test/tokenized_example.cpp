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
//[tokenized_example
#include <boost/range/adaptor/tokenized.hpp>
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
void tokenized_example_test()
//->
//=int main(int argc, const char* argv[])
{
    using namespace boost::adaptors;
    
    typedef boost::sub_match< std::string::iterator > match_type;
    
    std::string input = " a b c d e f g hijklmnopqrstuvwxyz";
    boost::copy(
        input | tokenized(boost::regex("\\w+")),
        std::ostream_iterator<match_type>(std::cout, "\n"));

//=    return 0;
//=}
//]
    using namespace boost::assign;

    std::vector<std::string> reference;
    reference += "a","b","c","d","e","f","g","hijklmnopqrstuvwxyz";
    
    std::vector<match_type> test;
    boost::push_back(test, input | tokenized(boost::regex("\\w+")));
    
    BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
        test.begin(), test.end() );
}
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.tokenized_example" );

    test->add( BOOST_TEST_CASE( &tokenized_example_test ) );

    return test;
}
