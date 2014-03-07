// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom Linestring Example

#include <iostream>
#include <string>
#include <vector>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>

// To register the 'geographic' distance function to calculate distance over the earth:
#include <boost/geometry/extensions/gis/geographic/strategies/andoyer.hpp>
#include <boost/geometry/extensions/algorithms/parse.hpp>

// Define a GPS point with coordinates in latitude/longitude and some additional values
struct gps_point
{
    double latitude, longitude, height;
    double speed;
    // Date/time, heading, etc could be added

    // The default constructor is required if being used in a vector
    gps_point() {}

    // Define a constructor to create the point in one line. Order of latitude/longitude
    // does not matter as long as "E", "N", etc are included
    gps_point(std::string const& c1, std::string const& c2, double h, double s)
        : height(h)
        , speed(s)
    {
        boost::geometry::parse(*this, c1, c2);
    }
};

// Declare a custom linestring which will have the GPS points
struct gps_track : std::vector<gps_point>
{
    std::string owner;
    int route_identifier;
    // etc

    gps_track(int i, std::string const& o)
        : owner(o)
        , route_identifier(i)
    {}
};


// Register this point as being a recognizable point by Boost.Geometry
BOOST_GEOMETRY_REGISTER_POINT_2D(gps_point, double, cs::geographic<degree>, longitude, latitude)

// Register the track as well, as being a "linestring"
BOOST_GEOMETRY_REGISTER_LINESTRING(gps_track)


int main()
{
    // Declare a "GPS Track" and add some GPS points
    gps_track track(23, "Mister G");
    track.push_back(gps_point("52 22 23 N", "4 53 32 E", 50, 180));
    track.push_back(gps_point("52 10 00 N", "4 59 59 E", 110, 170));
    track.push_back(gps_point("52 5 20 N", "5 6 56 E", 0, 90));

    std::cout
        << "track:  " << track.route_identifier << std::endl
        << "from:   " << track.owner << std::endl
        << "as wkt: " << boost::geometry::dsv(track) << std::endl
        << "length: " << boost::geometry::length(track)/1000.0 << " km" << std::endl;

    // Above gives the idea, shows that custom linestrings can be useful.
    // We could of course do anything with this track which the library can handle, e.g.:
    // - simplify it
    // - calculate distance of point-to-line
    // - project it to UTM, then transform it to a GIF image (see p03_example)

    return 0;
}
