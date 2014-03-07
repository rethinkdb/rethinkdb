/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_function.cpp
 * \author Andrey Semashev
 * \date   25.01.2009
 *
 * \brief  This header contains tests for the function attribute.
 */

#define BOOST_TEST_MODULE attr_function

#include <string>
#include <boost/mpl/vector.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/function.hpp>
#include <boost/log/utility/type_dispatch/static_type_dispatcher.hpp>

namespace logging = boost::log;
namespace attrs = logging::attributes;

namespace {

    // Type dispatcher for the supported types
    struct my_dispatcher :
        public logging::static_type_dispatcher<
            boost::mpl::vector< int, std::string >
        >
    {
        typedef logging::static_type_dispatcher<
            boost::mpl::vector< int, std::string >
        > base_type;

        enum type_expected
        {
            none_expected,
            int_expected,
            string_expected
        };

        my_dispatcher() : base_type(*this), m_Expected(none_expected), m_Int(0) {}

        void set_expected()
        {
            m_Expected = none_expected;
        }
        void set_expected(int value)
        {
            m_Expected = int_expected;
            m_Int = value;
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
        void operator() (std::string const& value) const
        {
            BOOST_CHECK_EQUAL(m_Expected, string_expected);
            BOOST_CHECK_EQUAL(m_String, value);
        }

    private:
        type_expected m_Expected;
        int m_Int;
        std::string m_String;
    };

    // A test function that returns an attribute value
    int get_attr_value()
    {
        return 10;
    }

    // A test functional object that returns an attribute value
    struct attr_value_generator
    {
        typedef std::string result_type;

        explicit attr_value_generator(unsigned int& count) : m_CallsCount(count) {}
        result_type operator() () const
        {
            ++m_CallsCount;
            return "Hello, world!";
        }

    private:
        unsigned int& m_CallsCount;
    };

} // namespace

// The test verifies that the attribute calls the specified functor and returns the value returned by the functor
BOOST_AUTO_TEST_CASE(calling)
{
    unsigned int call_count = 0;
    my_dispatcher disp;

    logging::attribute attr1 =
#ifndef BOOST_NO_RESULT_OF
        attrs::make_function(&get_attr_value);
#else
        attrs::make_function< int >(&get_attr_value);
#endif

    logging::attribute attr2 =
#ifndef BOOST_NO_RESULT_OF
        attrs::make_function(attr_value_generator(call_count));
#else
        attrs::make_function< std::string >(attr_value_generator(call_count));
#endif

    logging::attribute_value p1(attr1.get_value());
    logging::attribute_value p2(attr2.get_value());
    BOOST_CHECK_EQUAL(call_count, 1);
    logging::attribute_value p3(attr2.get_value());
    BOOST_CHECK_EQUAL(call_count, 2);

    disp.set_expected(10);
    BOOST_CHECK(p1.dispatch(disp));
    BOOST_CHECK(p1.dispatch(disp)); // check that the contained value doesn't change over time or upon dispatching

    disp.set_expected("Hello, world!");
    BOOST_CHECK(p2.dispatch(disp));
    BOOST_CHECK(p3.dispatch(disp));
}
