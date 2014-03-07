/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_functor_void_return.cpp
 * \author Andrey Semashev
 * \date   25.01.2009
 *
 * \brief  This test checks that it is not possible to create a functor attribute
 *         with a void-returning functor.
 */

#define BOOST_TEST_MODULE attr_functor_void_return

#include <boost/utility/result_of.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/function.hpp>

namespace logging = boost::log;
namespace attrs = logging::attributes;

namespace {

    // A test function that returns an attribute value
    void get_attr_value();

} // namespace

int main(int, char*[])
{
    logging::attribute attr1 =
#ifndef BOOST_NO_RESULT_OF
        attrs::make_function(&get_attr_value);
#else
        attrs::make_function< void >(&get_attr_value);
#endif

    return 0;
}
