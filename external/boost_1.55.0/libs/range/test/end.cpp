// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
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

#include <boost/range/end.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/included/unit_test.hpp>

namespace mock_std
{
    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME boost::range_iterator<SinglePassRange>::type
    end(SinglePassRange& rng)
    {
        return rng.end();
    }

    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME boost::range_iterator<const SinglePassRange>::type
    end(const SinglePassRange& rng)
    {
        return rng.end();
    }

    template<class SinglePassRange>
    void mock_algorithm_using_end(const SinglePassRange& rng)
    {
        BOOST_CHECK( end(rng) == rng.end() );
    }

    template<class SinglePassRange>
    void mock_algorithm_using_end(SinglePassRange& rng)
    {
        BOOST_CHECK( end(rng) == rng.end() );
    }
}

namespace boost
{
#ifdef BOOST_RANGE_SIMULATE_END_WITHOUT_ADL_NAMESPACE_BARRIER
    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange>::type
    end(SinglePassRange& rng)
    {
        return rng.end();
    }

    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange>::type
    end(const SinglePassRange& rng)
    {
        return rng.end();
    }
#endif

    class MockTestEndCollection
    {
    public:
        typedef char value_type;
        typedef const char* const_pointer;
        typedef char* pointer;
        typedef const_pointer const_iterator;
        typedef pointer iterator;

        MockTestEndCollection()
            : m_first()
            , m_last()
        {
        }

        const_iterator begin() const { return m_first; }
        iterator begin() { return m_first; }
        const_iterator end() const { return m_last; }
        iterator end() { return m_last; }

    private:
        iterator m_first;
        iterator m_last;
    };
}

namespace
{
    void test_range_end_adl_avoidance()
    {
        boost::MockTestEndCollection c;
        const boost::MockTestEndCollection& const_c = c;
        mock_std::mock_algorithm_using_end(const_c);
        mock_std::mock_algorithm_using_end(c);
    }
}

using boost::unit_test::test_suite;

boost::unit_test::test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE( "Range Test Suite - end() ADL namespace barrier" );

    test->add( BOOST_TEST_CASE( &test_range_end_adl_avoidance ) );

    return test;
}


