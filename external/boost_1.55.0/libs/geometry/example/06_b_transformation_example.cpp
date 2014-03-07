// Boost.Geometry (aka GGL, Generic Geometry Library)
// Example: Affine Transformation (translate, scale, rotate)
//
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <ctime> // for std::time
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/strategies/transform.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>
#include <boost/geometry/io/wkt/read.hpp>

#if defined(HAVE_SVG)
#  include <boost/geometry/io/svg/write_svg.hpp>
#endif

#include <boost/bind.hpp>
#include <boost/random.hpp>
#include <boost/range.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost::geometry;

struct random_style
{
    random_style()
        : rng(static_cast<int>(std::time(0))), dist(0, 255), colour(rng, dist)
    {}

    std::string fill(double opacity = 1)
    {
        std::ostringstream oss;
        oss << "fill:rgba(" << colour() << "," << colour() << "," << colour() << "," << opacity << ");";
        return oss.str();
    }

    std::string stroke(int width, double opacity = 1)
    {
        std::ostringstream oss;
        oss << "stroke:rgba(" << colour() << "," << colour() << "," << colour() << "," << opacity << ");";
        oss << "stroke-width:" << width  << ";";
        return oss.str();
    }

    template <typename T>
    std::string text(T x, T y, std::string const& text)
    {
        std::ostringstream oss;
        oss << "<text x=\"" << static_cast<int>(x) - 90 << "\" y=\"" << static_cast<int>(y) << "\" font-family=\"Verdana\">" << text << "</text>";
        return oss.str();
    }

    boost::mt19937 rng;
    boost::uniform_int<> dist;
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > colour;
};

template <typename OutputStream>
struct svg_output
{
    svg_output(OutputStream& os, double opacity = 1) : os(os), opacity(opacity)
    {
        os << "<?xml version=\"1.0\" standalone=\"no\"?>\n"
            << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n"
            << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
            << "<svg width=\"100%\" height=\"100%\" version=\"1.1\"\n"
            << "xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
    }

    ~svg_output()
    {
        os << "</svg>" << std::endl;
    }

    template <typename G>
    void put(G const& g, std::string const& label)
    {
        std::string style_str(style.fill(opacity) + style.stroke(5, opacity));
#if defined(HAVE_SVG)
        os << boost::geometry::svg(g, style_str) << std::endl;
#endif
        if (!label.empty())
        {
            typename point_type<G>::type c;
            centroid(g, c);
            os << style.text(static_cast<int>(get<0>(c)), static_cast<int>(get<1>(c)), label);
        }
    }

private:

    OutputStream& os;
    double opacity;
    random_style style;
};


int main()
{
    using namespace boost::geometry::strategy::transform;

    typedef boost::geometry::model::d2::point_xy<double> point_2d;

    try
    {
        std::string file("06_b_transformation_example.svg");
        std::ofstream ofs(file.c_str());
        svg_output<std::ofstream> svg(ofs, 0.5);

        // G1 - create subject for affine transformations
        model::polygon<point_2d> g1;
        read_wkt("POLYGON((50 250, 400 250, 150 50, 50 250))", g1);
        std::clog << "source box:\t" << boost::geometry::dsv(g1) << std::endl;
        svg.put(g1, "g1");

        // G1 - Translate -> G2
        translate_transformer<double, 2, 2> translate(0, 250);
        model::polygon<point_2d> g2;
        transform(g1, g2, translate);
        std::clog << "translated:\t" << boost::geometry::dsv(g2) << std::endl;
        svg.put(g2, "g2=g1.translate(0,250)");

        // G2 - Scale -> G3
        scale_transformer<double, 2, 2> scale(0.5, 0.5);
        model::polygon<point_2d> g3;
        transform(g2, g3, scale);
        std::clog << "scaled:\t" << boost::geometry::dsv(g3) << std::endl;
        svg.put(g3, "g3=g2.scale(0.5,0.5)");

        // G3 - Combine rotate and translate -> G4
        rotate_transformer<degree, double, 2, 2> rotate(45);

        // Compose matrix for the two transformation
        // Create transformer attached to the transformation matrix
        ublas_transformer<double, 2, 2>
                combined(boost::numeric::ublas::prod(rotate.matrix(), translate.matrix()));
                //combined(rotate.matrix());

        // Apply transformation to subject geometry point-by-point
        model::polygon<point_2d> g4;
        transform(g3, g4, combined);

        std::clog << "rotated & translated:\t" << boost::geometry::dsv(g4) << std::endl;
        svg.put(g4, "g4 = g3.(rotate(45) * translate(0,250))");

        std::clog << "Saved SVG file:\t" << file << std::endl;
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "unknown error" << std::endl;
    }
    return 0;
}
