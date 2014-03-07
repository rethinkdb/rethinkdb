// (c) Copyright Juergen Hunold 2012
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE QtQuick
#include <QDir>
#include <QTimer>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickView>
#include <QDebug>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE (defines)
{
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_QML_LIB), true);
    BOOST_CHECK_EQUAL(BOOST_IS_DEFINED(QT_QUICK_LIB), true);
}

BOOST_AUTO_TEST_CASE (simple_test)
{
    QGuiApplication app(boost::unit_test::framework::master_test_suite().argc,
                        boost::unit_test::framework::master_test_suite().argv);
    QQuickView view;

    QString fileName(boost::unit_test::framework::master_test_suite().argv[1]);

    view.connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));
    view.setSource(QUrl::fromLocalFile(fileName)); \

    QTimer::singleShot(2000, &app, SLOT(quit())); // Auto-close window

    if (QGuiApplication::platformName() == QLatin1String("qnx") ||
          QGuiApplication::platformName() == QLatin1String("eglfs")) {
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.showFullScreen();
    } else {
        view.show();
    }
    BOOST_CHECK_EQUAL(app.exec(), 0);
}
