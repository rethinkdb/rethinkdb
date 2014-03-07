// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// GD example

// GD is a well-known and often used graphic library to write GIF (and other formats)

// To build and run this example:
// 1) download GD from http://www.libgd.org (currently gd-2.0.35 is assumed)
// 2) add 11 files
//          gd.c, gd_gd.c, gd_gif_out.c, gd_io*.c, gd_security.c, gd_topal.c, gdhelpers.c, gdtables.c
//    to the project or makefile or jamfile
// 3) for windows, add define NONDLL to indicate GD is not used as a DLL
//    (Note that steps 2 and 3 are done in the MSVC gd_example project file and property sheets)

#include <cmath>
#include <cstdio>
#include <vector>

#include <fstream>
#include <sstream>


#include <boost/foreach.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/extensions/gis/latlong/latlong.hpp>
#include <boost/geometry/extensions/gis/geographic/strategies/area_huiller_earth.hpp>


#include <gd.h>

namespace bg = boost::geometry;


// ----------------------------------------------------------------------------
// Read an ASCII file containing WKT's
// (note this function is shared by various examples)
// ----------------------------------------------------------------------------
template <typename Geometry>
inline void read_wkt(std::string const& filename, std::vector<Geometry>& geometries)
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
                bg::read_wkt(line, geometry);
                geometries.push_back(geometry);
            }
        }
    }
}


int main()
{
    // Adapt if necessary
    std::string filename = "../data/world.wkt";


    // The world is measured in latlong (degrees), so latlong is appropriate.
    // We ignore holes in this sample (below)
    typedef bg::model::ll::point<bg::degree> point_type;
    typedef bg::model::polygon<point_type> polygon_type;
    typedef bg::model::multi_polygon<polygon_type> country_type;

    std::vector<country_type> countries;

    // Read (for example) world countries
    read_wkt(filename, countries);


    // Create a GD image.
    // A world map, as world.shp, is usually mapped in latitude-longitude (-180..180 and -90..90)
    // For this example we use a very simple "transformation"
    // mapping to 0..720 and 0..360
    const double factor = 2.0;
    gdImagePtr im = gdImageCreateTrueColor(int(360 * factor), int(180 * factor));

    // Allocate three colors
    int blue = gdImageColorResolve(im, 0, 52, 255);
    int green = gdImageColorResolve(im, 0, 255, 0);
    int black = gdImageColorResolve(im, 0, 0, 0);

    // Paint background in blue
    gdImageFilledRectangle(im, 0, 0, im->sx, im->sy, blue);

    // Paint all countries in green
    BOOST_FOREACH(country_type const& country, countries)
    {
        BOOST_FOREACH(polygon_type const& polygon, country)
        {
            // Ignore holes, so take only exterior ring
            bg::model::ring<point_type> const& ring = bg::exterior_ring(polygon);

            // If wished, suppress too small polygons.
            // (Note that even in latlong, area is calculated in square meters)
            double const a = bg::area(ring);
            if (std::fabs(a) > 5000.0e6)
            {
                int const n = ring.size();
                gdPoint* points = new gdPoint[n];

                for (int i = 0; i < n; i++)
                {
                    // Translate lon/lat or x/y to GD x/y points
                    points[i].x = int(factor * (bg::get<0>(ring[i]) + 180.0));
                    points[i].y = im->sy - int(factor * (bg::get<1>(ring[i]) + 90.0));
                }

                // Draw the polygon...
                gdImageFilledPolygon(im, points, n, green);
                // .. and the outline in black...
                gdImagePolygon(im, points, n, black);

                delete[] points;
            }
        }
    }

    // Use GD to create a GIF file
    std::FILE* out = std::fopen("world.gif", "wb");
    if (out != NULL)
    {
        gdImageGif(im, out);
        std::fclose(out);
    }

    gdImageDestroy(im);

    return 0;
}
