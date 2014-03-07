// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Qt Example

// Qt is a well-known and often used platform independent windows library

// To build and run this example:
// 1) download (from http://qt.nokia.com), configure and make QT
// 2) if necessary, adapt Qt clause in include path (note there is a Qt property sheet)

#include <sstream>

#include <QtGui>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>


// Adapt a QPointF such that it can be handled by Boost.Geometry
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(QPointF, double, cs::cartesian, x, y, setX, setY)

// Adapt a QPolygonF as well.
// A QPolygonF has no holes (interiors) so it is similar to a Boost.Geometry ring
BOOST_GEOMETRY_REGISTER_RING(QPolygonF)


int main(int argc, char *argv[])
{
    // This usage QApplication and QLabel is adapted from
    // http://en.wikipedia.org/wiki/Qt_(toolkit)#Qt_hello_world
    QApplication app(argc, argv);

    // Declare a Qt polygon. The Qt Polygon can be used
    // in Boost.Geometry, just by its oneline registration above.
    QPolygonF polygon;

    // Use Qt to add points to polygon
    polygon
        << QPointF(10, 20) << QPointF(20, 30)
        << QPointF(30, 20) << QPointF(20, 10)
        << QPointF(10, 20);

    // Use Boost.Geometry e.g. to calculate area
    std::ostringstream out;
    out << "Boost.Geometry area: " << boost::geometry::area(polygon) << std::endl;

    // Some functionality is defined in both Qt and Boost.Geometry
    QPointF p(20,20);
    out << "Qt contains: "
        << (polygon.containsPoint(p, Qt::WindingFill) ? "yes" : "no")
        << std::endl
        << "Boost.Geometry within: "
        << (boost::geometry::within(p, polygon) ? "yes" : "no")
        << std::endl;
    // Detail: if point is ON boundary, Qt says yes, Boost.Geometry says no.

    // Qt defines an iterator
    // (which is required for of the Boost.Geometry ring-concept)
    // such that Boost.Geometry can use the points of this polygon
    QPolygonF::const_iterator it;
    for (it = polygon.begin(); it != polygon.end(); ++it)
    {
        // Stream Delimiter-Separated, just to show something Boost.Geometry can do
        out << boost::geometry::dsv(*it) << std::endl;
    }

    // Stream the polygon as well
    out << boost::geometry::dsv(polygon) << std::endl;

    // Just show what we did in a label
    QLabel label(out.str().c_str());
    label.show();
    return app.exec();
}

