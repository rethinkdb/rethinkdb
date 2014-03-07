// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Qt World Mapper Example

// Qt is a well-known and often used platform independent windows library

// To build and run this example:
// 1) download (from http://qt.nokia.com), configure and make QT
// 2) if necessary, adapt Qt clause in include path (note there is a Qt property sheet)

#include <fstream>

#include <QtGui>
#include <QWidget>
#include <QObject>
#include <QPainter>

#include <boost/foreach.hpp>

#include <boost/geometry/geometry.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>

#include <boost/geometry/extensions/algorithms/selected.hpp>



// Adapt a QPointF such that it can be handled by Boost.Geometry
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(QPointF, double, cs::cartesian, x, y, setX, setY)

// Adapt a QPolygonF as well.
// A QPolygonF has no holes (interiors) so it is similar to a Boost.Geometry ring
BOOST_GEOMETRY_REGISTER_RING(QPolygonF)


typedef boost::geometry::model::d2::point_xy<double> point_2d;
typedef boost::geometry::model::multi_polygon
    <
        boost::geometry::model::polygon<point_2d>
    > country_type;


class WorldMapper : public QWidget
{
 public:
    WorldMapper(std::vector<country_type> const& countries, boost::geometry::model::box<point_2d> const& box)
        : m_countries(countries)
        , m_box(box)
    {
        setPalette(QPalette(QColor(200, 250, 250)));
        setAutoFillBackground(true);
    }

 protected:
    void paintEvent(QPaintEvent*)
    {
        map_transformer_type transformer(m_box, this->width(), this->height());

        QPainter painter(this);
        painter.setBrush(Qt::green);
        painter.setRenderHint(QPainter::Antialiasing);

        BOOST_FOREACH(country_type const& country, m_countries)
        {
            typedef boost::range_value<country_type>::type polygon_type;
            BOOST_FOREACH(polygon_type const& polygon, country)
            {
                typedef boost::geometry::ring_type<polygon_type>::type ring_type;
                ring_type const& ring = boost::geometry::exterior_ring(polygon);

                // This is the essention:
                // Directly transform from a multi_polygon (ring-type) to a QPolygonF
                QPolygonF qring;
                boost::geometry::transform(ring, qring, transformer);

                painter.drawPolygon(qring);
            }
        }
    }

 private:
    typedef boost::geometry::strategy::transform::map_transformer
        <
            double, 2, 2,
            true, true
        > map_transformer_type;

     std::vector<country_type> const& m_countries;
     boost::geometry::model::box<point_2d> const& m_box;
 };


class MapperWidget : public QWidget
{
 public:
     MapperWidget(std::vector<country_type> const& countries, boost::geometry::model::box<point_2d> const& box, QWidget *parent = 0)
         : QWidget(parent)
     {
         WorldMapper* mapper = new WorldMapper(countries, box);

         QPushButton *quit = new QPushButton(tr("Quit"));
         quit->setFont(QFont("Times", 18, QFont::Bold));
         connect(quit, SIGNAL(clicked()), qApp, SLOT(quit()));

         QVBoxLayout *layout = new QVBoxLayout;
         layout->addWidget(mapper);
         layout->addWidget(quit);
         setLayout(layout);
     }

};


// ----------------------------------------------------------------------------
// Read an ASCII file containing WKT's
// ----------------------------------------------------------------------------
template <typename Geometry, typename Box>
inline void read_wkt(std::string const& filename, std::vector<Geometry>& geometries, Box& box)
{
    std::ifstream cpp_file(filename.c_str());
    if (cpp_file.is_open())
    {
        while (! cpp_file.eof() )
        {
            std::string line;
            std::getline(cpp_file, line);
            if (! line.empty())
            {
                Geometry geometry;
                boost::geometry::read_wkt(line, geometry);
                geometries.push_back(geometry);
                boost::geometry::expand(box, boost::geometry::return_envelope<Box>(geometry));
            }
        }
    }
}


int main(int argc, char *argv[])
{
    std::vector<country_type> countries;
    boost::geometry::model::box<point_2d> box;
    boost::geometry::assign_inverse(box);
    read_wkt("../data/world.wkt", countries, box);

    QApplication app(argc, argv);
    MapperWidget widget(countries, box);
    widget.setWindowTitle("Boost.Geometry for Qt - Hello World!");
    widget.setGeometry(50, 50, 800, 500);
    widget.show();
    return app.exec();
}
