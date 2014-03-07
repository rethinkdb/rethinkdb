/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_comparison.hpp
 * \author Andrey Semashev
 * \date   06.08.2010
 *
 * \brief  This header contains tools for attribute comparison in attribute-related tests.
 */

#ifndef BOOST_LOG_TESTS_ATTR_COMPARISON_HPP_INCLUDED_
#define BOOST_LOG_TESTS_ATTR_COMPARISON_HPP_INCLUDED_

#include <ostream>
#include <boost/log/attributes/attribute.hpp>

class attribute_factory_helper :
    public boost::log::attribute
{
    typedef boost::log::attribute base_type;

public:
    attribute_factory_helper(base_type const& that) : base_type(that)
    {
    }

    impl* get_impl() const
    {
        return base_type::get_impl();
    }
};

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

inline bool operator== (attribute const& left, attribute const& right)
{
    return attribute_factory_helper(left).get_impl() == attribute_factory_helper(right).get_impl();
}
inline bool operator!= (attribute const& left, attribute const& right)
{
    return attribute_factory_helper(left).get_impl() != attribute_factory_helper(right).get_impl();
}

inline std::ostream& operator<< (std::ostream& strm, attribute const& val)
{
    strm << attribute_factory_helper(val).get_impl();
    return strm;
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_TESTS_ATTR_COMPARISON_HPP_INCLUDED_
