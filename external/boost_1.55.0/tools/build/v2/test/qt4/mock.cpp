// (c) Copyright Juergen Hunold 2011
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE QtMoc

#include "mock.h"

#include <boost/test/unit_test.hpp>

Mock::Mock()
{
}

/*!
  Check that the compiler get the correct #defines.
  The logic to test the moc is in the header file "mock.h"
 */
BOOST_AUTO_TEST_CASE(construct_mock)
{
    delete new Mock();

    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(TEST_MOCK), true);
}
