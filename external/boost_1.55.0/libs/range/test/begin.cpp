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

#include <boost/range/begin.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/included/unit_test.hpp>

namespace mock_std
{
    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME boost::range_iterator<SinglePassRange>::type
    begin(SinglePassRange& rng)
    {
        return rng.begin();
    }

    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME boost::range_iterator<const SinglePassRange>::type
    begin(const SinglePassRange& rng)
    {
        return rng.begin();
    }

    template<class SinglePassRange>
    void mock_algorithm_using_begin(const SinglePassRange& rng)
    {
        BOOST_CHECK( begin(rng) == rng.begin() );
    }

    template<class SinglePassRange>
    void mock_algorithm_using_begin(SinglePassRange& rng)
    {
        BOOST_CHECK( begin(rng) == rng.begin() );
    }
}

namespace boost
{
#ifdef BOOST_RANGE_SIMULATE_BEGIN_WITHOUT_ADL_NAMESPACE_BARRIER
    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange>::type
    begin(SinglePassRange& rng)
    {
        return rng.begin();
    }

    template<class SinglePassRange>
    inline BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange>::type
    begin(const SinglePassRange& rng)
    {
        return rng.begin();
    }
#endif

    class MockTestBeginCollection
    {
    public:
        typedef char value_type;
        typedef const char* const_pointer;
        typedef char* pointer;
        typedef const_pointer const_iterator;
        typedef pointer iterator;

        MockTestBeginCollection()
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
    void test_range_begin()
    {
        boost::MockTestBeginCollection c;
        const boost::MockTestBeginCollection& const_c = c;
        mock_std::mock_algorithm_using_begin(const_c);
        mock_std::mock_algorithm_using_begin(c);
    }
}

using boost::unit_test::test_suite;

boost::unit_test::test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE( "Range Test Suite - begin() ADL namespace barrier" );

    test->add( BOOST_TEST_CASE( &test_range_begin ) );

    return test;
}


