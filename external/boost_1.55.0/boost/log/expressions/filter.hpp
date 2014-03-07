/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   filter.hpp
 * \author Andrey Semashev
 * \date   13.07.2012
 *
 * The header contains a filter function object definition.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FILTER_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FILTER_HPP_INCLUDED_

#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * Log record filter function wrapper.
 */
class filter
{
    BOOST_COPYABLE_AND_MOVABLE(filter)

public:
    //! Result type
    typedef bool result_type;

private:
    //! Filter function type
    typedef boost::log::aux::light_function< bool (attribute_value_set const&) > filter_type;

    //! Default filter, always returns \c true
    struct default_filter
    {
        typedef bool result_type;
        result_type operator() (attribute_value_set const&) const { return true; }
    };

private:
    //! Filter function
    filter_type m_Filter;

public:
    /*!
     * Default constructor. Creates a filter that always returns \c true.
     */
    filter() : m_Filter(default_filter())
    {
    }
    /*!
     * Copy constructor
     */
    filter(filter const& that) : m_Filter(that.m_Filter)
    {
    }
    /*!
     * Move constructor. The moved-from filter is left in an unspecified state.
     */
    filter(BOOST_RV_REF(filter) that) BOOST_NOEXCEPT : m_Filter(boost::move(that.m_Filter))
    {
    }

    /*!
     * Initializing constructor. Creates a filter which will invoke the specified function object.
     */
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    filter(FunT const& fun)
#else
    template< typename FunT >
    filter(FunT const& fun, typename disable_if< move_detail::is_rv< FunT >, int >::type = 0)
#endif
        : m_Filter(fun)
    {
    }

    /*!
     * Move assignment. The moved-from filter is left in an unspecified state.
     */
    filter& operator= (BOOST_RV_REF(filter) that) BOOST_NOEXCEPT
    {
        m_Filter.swap(that.m_Filter);
        return *this;
    }
    /*!
     * Copy assignment.
     */
    filter& operator= (BOOST_COPY_ASSIGN_REF(filter) that)
    {
        m_Filter = that.m_Filter;
        return *this;
    }
    /*!
     * Initializing assignment. Sets the specified function object to the filter.
     */
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    filter& operator= (FunT const& fun)
#else
    template< typename FunT >
    typename disable_if< is_same< typename remove_cv< FunT >::type, filter >, filter& >::type
    operator= (FunT const& fun)
#endif
    {
        filter(fun).swap(*this);
        return *this;
    }

    /*!
     * Filtering operator.
     *
     * \param values Attribute values of the log record.
     * \return \c true if the log record passes the filter, \c false otherwise.
     */
    result_type operator() (attribute_value_set const& values) const
    {
        return m_Filter(values);
    }

    /*!
     * Resets the filter to the default. The default filter always returns \c true.
     */
    void reset()
    {
        m_Filter = default_filter();
    }

    /*!
     * Swaps two filters
     */
    void swap(filter& that) BOOST_NOEXCEPT
    {
        m_Filter.swap(that.m_Filter);
    }
};

inline void swap(filter& left, filter& right) BOOST_NOEXCEPT
{
    left.swap(right);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FILTER_HPP_INCLUDED_
