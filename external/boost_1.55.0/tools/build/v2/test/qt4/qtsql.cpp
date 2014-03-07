// (c) Copyright Juergen Hunold 2008
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE QtSql

#include <QSqlDatabase>

#include <QTextStream>
#include <QStringList>

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_SQL_LIB), true);
}

BOOST_AUTO_TEST_CASE( drivers )
{
    QTextStream stream(stdout, QIODevice::WriteOnly);

    Q_FOREACH(QString it, QSqlDatabase:: drivers())
    {
        stream << it << endl;
    }
}

BOOST_AUTO_TEST_CASE( construct )
{
    QSqlDatabase database;
    BOOST_CHECK_EQUAL(database.isOpen(), false);
}
