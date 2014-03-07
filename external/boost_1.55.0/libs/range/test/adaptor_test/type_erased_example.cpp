// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
//[type_erased_example
#include <boost/range/adaptor/type_erased.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <iterator>
#include <iostream>
#include <list>
#include <vector>
//<-
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

namespace
{
    namespace boost_range_test
    {
        namespace type_erased_example
        {
//->            

// The client interface from an OO perspective merely requires a sequence
// of integers that can be forward traversed
typedef boost::any_range<
    int
  , boost::forward_traversal_tag
  , int
  , std::ptrdiff_t
> integer_range;

namespace server
{
    void display_integers(const integer_range& rng)
    {
        boost::copy(rng,
                    std::ostream_iterator<int>(std::cout, ","));

        std::cout << std::endl;
    }
}

namespace client
{
    void run()
    {
        using namespace boost::assign;
        using namespace boost::adaptors;

        // Under most conditions one would simply use an appropriate
        // any_range as a function parameter. The type_erased adaptor
        // is often superfluous. However because the type_erased
        // adaptor is applied to a range, we can use default template
        // arguments that are generated in conjunction with the
        // range type to which we are applying the adaptor.

        std::vector<int> input;
        input += 1,2,3,4,5;

        // Note that this call is to a non-template function
        server::display_integers(input);

        std::list<int> input2;
        input2 += 6,7,8,9,10;

        // Note that this call is to the same non-tempate function
        server::display_integers(input2);

        input2.clear();
        input2 += 11,12,13,14,15;

        // Calling using the adaptor looks like this:
        // Notice that here I have a type_erased that would be a
        // bidirectional_traversal_tag, but this is convertible
        // to the forward_traversal_tag equivalent hence this
        // works.
        server::display_integers(input2 | type_erased<>());

        // However we may simply wish to define an adaptor that
        // takes a range and makes it into an appropriate
        // forward_traversal any_range...
        typedef boost::adaptors::type_erased<
            boost::use_default
          , boost::forward_traversal_tag
        > type_erased_forward;

        // This adaptor can turn other containers with different
        // value_types and reference_types into the appropriate
        // any_range.

        server::display_integers(input2 | type_erased_forward());
    }
}

//=int main(int argc, const char* argv[])
//={
//=    client::run();
//=    return 0;
//=}
//]

        } // namespace type_erased_example
    } // namespace boost_range_test
} // anonymous namespace

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.type_erased_example" );

    test->add( BOOST_TEST_CASE( &boost_range_test::type_erased_example::client::run) );

    return test;
}
