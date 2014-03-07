/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   make_record.hpp
 * \author Andrey Semashev
 * \date   18.03.2009
 *
 * \brief  This header contains a helper function make_record that creates a log record with the specified attributes.
 */

#ifndef BOOST_LOG_TESTS_MAKE_RECORD_HPP_INCLUDED_
#define BOOST_LOG_TESTS_MAKE_RECORD_HPP_INCLUDED_

#include <boost/move/utility.hpp>
#include <boost/log/core.hpp>
#include <boost/log/attributes/attribute_set.hpp>

inline boost::log::record make_record(boost::log::attribute_set const& src_attrs)
{
    return boost::log::core::get()->open_record(src_attrs);
}

inline boost::log::record_view make_record_view(boost::log::attribute_set const& src_attrs)
{
    return make_record(src_attrs).lock();
}

#endif // BOOST_LOG_TESTS_MAKE_RECORD_HPP_INCLUDED_
