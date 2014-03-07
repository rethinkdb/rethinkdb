/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_attribute_value_impl.cpp
 * \author Andrey Semashev
 * \date   25.01.2009
 *
 * \brief  This header contains tests for the basic attribute value class.
 */

#define BOOST_TEST_MODULE attr_attribute_value_impl

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/utility/type_dispatch/static_type_dispatcher.hpp>
#include <boost/log/utility/functional/bind_assign.hpp>

namespace logging = boost::log;
namespace attrs = logging::attributes;

namespace {

    // Type dispatcher for the supported types
    struct my_dispatcher :
        public logging::static_type_dispatcher<
            boost::mpl::vector< int, double, std::string >
        >
    {
        typedef logging::static_type_dispatcher<
            boost::mpl::vector< int, double, std::string >
        > base_type;

        enum type_expected
        {
            none_expected,
            int_expected,
            double_expected,
            string_expected
        };

        my_dispatcher() :
            base_type(*this),
            m_Expected(none_expected),
            m_Int(0),
            m_Double(0.0)
        {
        }

        void set_expected()
        {
            m_Expected = none_expected;
        }
        void set_expected(int value)
        {
            m_Expected = int_expected;
            m_Int = value;
        }
        void set_expected(double value)
        {
            m_Expected = double_expected;
            m_Double = value;
        }
        void set_expected(std::string const& value)
        {
            m_Expected = string_expected;
            m_String = value;
        }

        // Implement visitation logic for all supported types
        void operator() (int const& value) const
        {
            BOOST_CHECK_EQUAL(m_Expected, int_expected);
            BOOST_CHECK_EQUAL(m_Int, value);
        }
        void operator() (double const& value) const
        {
            BOOST_CHECK_EQUAL(m_Expected, double_expected);
            BOOST_CHECK_CLOSE(m_Double, value, 0.001);
        }
        void operator() (std::string const& value) const
        {
            BOOST_CHECK_EQUAL(m_Expected, string_expected);
            BOOST_CHECK_EQUAL(m_String, value);
        }

    private:
        type_expected m_Expected;
        int m_Int;
        double m_Double;
        std::string m_String;
    };

} // namespace

// The test verifies that type dispatching works
BOOST_AUTO_TEST_CASE(type_dispatching)
{
    my_dispatcher disp;
    logging::attribute_value p1(attrs::make_attribute_value< int >(10));
    logging::attribute_value p2(attrs::make_attribute_value< double > (5.5));
    logging::attribute_value p3(attrs::make_attribute_value< std::string >(std::string("Hello, world!")));
    logging::attribute_value p4(attrs::make_attribute_value< float >(static_cast< float >(-7.2)));

    disp.set_expected(10);
    BOOST_CHECK(p1.dispatch(disp));
    BOOST_CHECK(p1.dispatch(disp)); // check that the contained value doesn't change over time or upon dispatching

    disp.set_expected(5.5);
    BOOST_CHECK(p2.dispatch(disp));

    disp.set_expected("Hello, world!");
    BOOST_CHECK(p3.dispatch(disp));

    disp.set_expected();
    BOOST_CHECK(!p4.dispatch(disp));
}

// The test verifies that value extraction works
BOOST_AUTO_TEST_CASE(value_extraction)
{
    logging::attribute_value p1(attrs::make_attribute_value< int >(10));
    logging::attribute_value p2(attrs::make_attribute_value< double >(5.5));

    logging::value_ref< int > val1 = p1.extract< int >();
    BOOST_CHECK(!!val1);
    BOOST_CHECK_EQUAL(val1.get(), 10);

    logging::value_ref< double > val2 = p1.extract< double >();
    BOOST_CHECK(!val2);

    double val3 = 0.0;
    BOOST_CHECK(p2.visit< double >(logging::bind_assign(val3)));
    BOOST_CHECK_CLOSE(val3, 5.5, 0.001);
}

// The test verifies that the detach_from_thread returns a valid pointer
BOOST_AUTO_TEST_CASE(detaching_from_thread)
{
    boost::intrusive_ptr< logging::attribute_value::impl > p1(new attrs::attribute_value_impl< int >(10));
    boost::intrusive_ptr< logging::attribute_value::impl > p2 = p1->detach_from_thread();
    BOOST_CHECK(!!p2);
}
