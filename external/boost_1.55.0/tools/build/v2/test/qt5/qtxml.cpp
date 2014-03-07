// (c) Copyright Juergen Hunold 2012
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE QtXml

#include <QtXml>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_XML_LIB), true);
}

BOOST_AUTO_TEST_CASE( reader_construct)
{
    QXmlStreamReader reader;
    BOOST_CHECK_EQUAL(reader.atEnd(), false);
}

BOOST_AUTO_TEST_CASE( writer_construct)
{
    QXmlStreamWriter writer;
    BOOST_CHECK_EQUAL(writer.device(), static_cast<QIODevice*>(0));
}

