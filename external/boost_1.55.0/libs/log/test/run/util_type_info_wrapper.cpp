/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   util_type_info_wrapper.cpp
 * \author Andrey Semashev
 * \date   18.01.2009
 *
 * \brief  This header contains tests for the type_info wrapper.
 */

#define BOOST_TEST_MODULE util_type_info_wrapper

#include <string>
#include <typeinfo>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <boost/log/utility/type_info_wrapper.hpp>

namespace logging = boost::log;

namespace {

    struct my_struct1 {};
    struct my_struct2 {};

} // namespace

// Tests for constructors and assignment
BOOST_AUTO_TEST_CASE(constructors)
{
    logging::type_info_wrapper info1;
    BOOST_CHECK(!info1);

    logging::type_info_wrapper info2 = typeid(my_struct1);
    BOOST_CHECK(info2);

    logging::type_info_wrapper info3 = info2;
    BOOST_CHECK(info3);

    logging::type_info_wrapper info4 = info1;
    BOOST_CHECK(!info4);

    info3 = info4;
    BOOST_CHECK(!info3);
}

// Tests for swapping
BOOST_AUTO_TEST_CASE(swapping)
{
    logging::type_info_wrapper info1 = typeid(my_struct1);
    logging::type_info_wrapper info2 = typeid(my_struct2);

    BOOST_CHECK(info1 == typeid(my_struct1));
    BOOST_CHECK(info2 == typeid(my_struct2));

    using std::swap;
    swap(info1, info2);
    BOOST_CHECK(info1 == typeid(my_struct2));
    BOOST_CHECK(info2 == typeid(my_struct1));

    info1.swap(info2);
    BOOST_CHECK(info1 == typeid(my_struct1));
    BOOST_CHECK(info2 == typeid(my_struct2));
}

// Tests for comparison and ordering
BOOST_AUTO_TEST_CASE(comparison)
{
    logging::type_info_wrapper info1 = typeid(my_struct1);
    logging::type_info_wrapper info2 = typeid(my_struct2);

    const bool ordering = typeid(my_struct1).before(typeid(my_struct2)) != 0;
    BOOST_CHECK((info1 < info2) == ordering);

    logging::type_info_wrapper empty1, empty2;
    BOOST_CHECK(empty1 != info1);
    BOOST_CHECK(empty1 != info2);
    BOOST_CHECK(empty1 == empty2);
}

// Tests for pretty_name
BOOST_AUTO_TEST_CASE(pretty_name)
{
    // We don't check for the prety_name result contents.
    // Simply verify that the function exists and returns a non-empty string.
    logging::type_info_wrapper info = typeid(my_struct1);
    std::string str = info.pretty_name();
    BOOST_CHECK(!str.empty());

    logging::type_info_wrapper empty;
    str = info.pretty_name();
    BOOST_CHECK(!str.empty());
}
