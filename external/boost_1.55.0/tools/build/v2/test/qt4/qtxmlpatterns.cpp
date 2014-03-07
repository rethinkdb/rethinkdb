// (c) Copyright Juergen Hunold 2008
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE QtXmlPatterns

#include <QXmlQuery>
#include <QXmlSerializer>

#include <QCoreApplication>
#include <QString>
#include <QTextStream>
#include <QBuffer>

#include <boost/test/unit_test.hpp>


struct Fixture
{
    Fixture()
        : application(boost::unit_test::framework::master_test_suite().argc,
                      boost::unit_test::framework::master_test_suite().argv)
    {
        BOOST_TEST_MESSAGE( "setup QCoreApplication fixture" );
    }

    ~Fixture()
    {
        BOOST_TEST_MESSAGE( "teardown QCoreApplication fixture" );
    }

    QCoreApplication application;
};

BOOST_GLOBAL_FIXTURE( Fixture )

QByteArray doc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<html xmlns=\"http://www.w3.org/1999/xhtml/\" xml:lang=\"en\" lang=\"en\">"
"   <head>"
"      <title>Global variables report for globals.gccxml</title>"
"   </head>"
"<body><p>Some Test text</p></body></html>");

BOOST_AUTO_TEST_CASE( defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_CORE_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_XMLPATTERNS_LIB), true);

    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_XML_LIB), false);
}

BOOST_AUTO_TEST_CASE( extract )
{

     QBuffer buffer(&doc); // This is a QIODevice.
     buffer.open(QIODevice::ReadOnly);
     QXmlQuery query;
     query.bindVariable("myDocument", &buffer);
     query.setQuery("declare variable $myDocument external;"
                    "doc($myDocument)");///p[1]");

     BOOST_CHECK_EQUAL(query.isValid(), true);

     QByteArray result;
     QBuffer out(&result);
     out.open(QIODevice::WriteOnly);
     
     QXmlSerializer serializer(query, &out);
     BOOST_CHECK_EQUAL(query.evaluateTo(&serializer), true);

     QTextStream stream(stdout);
     BOOST_CHECK_EQUAL(result.isEmpty(), false);
     stream << "hallo" << result << endl;
}

