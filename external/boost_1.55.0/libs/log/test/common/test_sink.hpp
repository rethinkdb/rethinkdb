/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   test_sink.hpp
 * \author Andrey Semashev
 * \date   18.03.2009
 *
 * \brief  This header contains a test sink frontend that is used through various tests.
 */

#ifndef BOOST_LOG_TESTS_TEST_SINK_HPP_INCLUDED_
#define BOOST_LOG_TESTS_TEST_SINK_HPP_INCLUDED_

#include <cstddef>
#include <map>
#include <boost/log/core/record_view.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/sinks/sink.hpp>
#include <boost/log/expressions/filter.hpp>

//! A sink implementation for testing purpose
struct test_sink :
    public boost::log::sinks::sink
{
public:
    typedef boost::log::attribute_value_set attribute_values;
    typedef boost::log::record_view record_type;
    typedef boost::log::filter filter_type;
    typedef attribute_values::key_type key_type;

    struct key_type_order
    {
        typedef bool result_type;

        result_type operator() (key_type const& left, key_type const& right) const
        {
            return left.id() < right.id();
        }
    };

    typedef std::map< key_type, std::size_t, key_type_order > attr_counters_map;

public:
    filter_type m_Filter;
    attr_counters_map m_Consumed;
    std::size_t m_RecordCounter;

public:
    test_sink() : boost::log::sinks::sink(false), m_RecordCounter(0) {}

    void set_filter(filter_type const& f)
    {
        m_Filter = f;
    }

    void reset_filter()
    {
        m_Filter.reset();
    }

    bool will_consume(attribute_values const& attributes)
    {
        return m_Filter(attributes);
    }

    void consume(record_type const& record)
    {
        ++m_RecordCounter;
        attribute_values::const_iterator
            it = record.attribute_values().begin(),
            end = record.attribute_values().end();
        for (; it != end; ++it)
            ++m_Consumed[it->first];
    }

    void flush()
    {
    }

    void clear()
    {
        m_RecordCounter = 0;
        m_Consumed.clear();
    }
};

#endif // BOOST_LOG_TESTS_TEST_SINK_HPP_INCLUDED_
