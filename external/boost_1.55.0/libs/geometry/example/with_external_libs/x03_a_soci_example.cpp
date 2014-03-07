// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// SOCI example

// a: using boost::tuple to retrieve points

// SOCI is a generic C++ template interface to access relational databases

// To build and run this example:
// 1) download SOCI from http://soci.sourceforge.net/
// 2) put it in contrib/soci-3.0.0 (or another version/folder, but then update this VCPROJ)
// 3) adapt your makefile or use this VCPROJ file
//    (note that SOCI sources are included directly, building SOCI is not necessary)
// 4) load the demo-data, see script data/cities.sql (for PostgreSQL)

#include <soci.h>
#include <soci-postgresql.h>

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/timer.hpp>
#include <boost/random.hpp>
#include <boost/tuple/tuple.hpp>

#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <exception>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian);


int main()
{
    try
    {
        soci::session sql(soci::postgresql, "dbname=ggl user=ggl password=ggl");

        int count;
        sql << "select count(*) from cities", soci::into(count);
        std::cout << "# Capitals: " << count << std::endl;

        typedef std::vector<boost::tuple<double, double> > V;

        soci::rowset<boost::tuple<double, double> > rows
            = sql.prepare << "select x(location),y(location) from cities";
        V vec;
        std::copy(rows.begin(), rows.end(), std::back_inserter(vec));

        for (V::const_iterator it = vec.begin(); it != vec.end(); ++it)
        {
            std::cout << it->get<0>() << " " << it->get<1>() << std::endl;
        }
        // Calculate distances
        for (V::const_iterator it1 = vec.begin(); it1 != vec.end(); ++it1)
        {
            for (V::const_iterator it2 = vec.begin(); it2 != vec.end(); ++it2)
            {
                std::cout << boost::geometry::dsv(*it1) << " " << boost::geometry::distance(*it1, *it2) << std::endl;
            }
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return 0;
}
