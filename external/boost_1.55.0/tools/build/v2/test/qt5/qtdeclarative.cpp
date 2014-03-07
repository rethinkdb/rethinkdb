// (c) Copyright Juergen Hunold 2012
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE QtDeclarative

#include <QCoreApplication>
#include <QDeclarativeView>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_XML_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_DECLARATIVE_LIB), true);
}


BOOST_AUTO_TEST_CASE( declarative )
{
    QCoreApplication app(boost::unit_test::framework::master_test_suite().argc,
                         boost::unit_test::framework::master_test_suite().argv);
    QDeclarativeView view;
}
