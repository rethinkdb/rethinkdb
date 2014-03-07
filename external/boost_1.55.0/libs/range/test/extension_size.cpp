// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//


#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // suppress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/range.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <list>
#include <vector>

namespace boost_range_extension_size_test
{
    class FooWithoutSize
    {
        typedef std::list<int> impl_t;
        typedef impl_t::const_iterator const_iterator;
        typedef impl_t::iterator iterator;

    public:
        friend inline const_iterator range_begin(const FooWithoutSize& obj) { return obj.m_impl.begin(); }
        friend inline iterator range_begin(FooWithoutSize& obj) { return obj.m_impl.begin(); }
        friend inline const_iterator range_end(const FooWithoutSize& obj) { return obj.m_impl.end(); }
        friend inline iterator range_end(FooWithoutSize& obj){ return obj.m_impl.end(); }

    private:
        impl_t m_impl;
    };

    inline boost::range_size<std::list<int> >::type
    range_calculate_size(const FooWithoutSize& rng)
    {
        return 2u;
    }
}

namespace boost
{
    template<> struct range_iterator<const ::boost_range_extension_size_test::FooWithoutSize>
    {
        typedef std::list<int>::const_iterator type;
    };

    template<> struct range_iterator< ::boost_range_extension_size_test::FooWithoutSize>
    {
        typedef std::list<int>::iterator type;
    };
}

namespace
{

void check_size_works_with_random_access()
{
    std::vector<int> container;
    container.push_back(1);
    BOOST_CHECK_EQUAL( boost::size(container), 1u );
}

void check_extension_size()
{
    BOOST_CHECK_EQUAL( boost::size(boost_range_extension_size_test::FooWithoutSize()), 2u );
}

} // anonymous namespace

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Range Test Suite" );

    test->add( BOOST_TEST_CASE( &check_size_works_with_random_access ));
    test->add( BOOST_TEST_CASE( &check_extension_size ) );

    return test;
}
