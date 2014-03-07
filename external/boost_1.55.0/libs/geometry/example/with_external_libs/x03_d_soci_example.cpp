// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Boost.Geometry (aka GGL, Generic Geometry Library)
// SOCI example

// d: using WKB to retrieve geometries

// SOCI is a generic C++ template interface to access relational databases

// To build and run this example, see comments in example a
// Alternatively compile composing and executing compiler command directoy in examples directory,
//    for example using GCC compiler:
//    g++ -I../../../boost -I/home/mloskot/usr/include/soci \
//        -I /home/mloskot/usr/include/soci/postgresql -I/usr/include/postgresql \
//        -L/home/mloskot/usr/lib -lsoci_core-gcc-3_0 -lsoci_postgresql-gcc-3_0 x03_c_soci_example.cpp

#include <soci.h>
#include <soci-postgresql.h>

#include <exception>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <boost/geometry.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/extensions/gis/io/wkb/read_wkb.hpp>
#include <boost/geometry/extensions/gis/io/wkb/utility.hpp>

int main()
{
    try
    {
        // establish database connection
        soci::session sql(soci::postgresql, "dbname=ggl user=ggl password=ggl");

        // construct schema of table for trees (point geometries)
        sql << "DELETE FROM geometry_columns WHERE f_table_name = 'parcels'";
        sql << "DROP TABLE IF EXISTS parcels CASCADE";
        sql << "CREATE TABLE parcels (id INTEGER)";
        sql << "SELECT AddGeometryColumn('parcels', 'geom', -1, 'GEOMETRY', 2)";

        // insert sample data using plain WKT input
        sql << "INSERT INTO parcels VALUES(1, ST_GeomFromText('POLYGON ((10 10, 10 20, 20 20, 20 15, 10 10))', -1))";
        sql << "INSERT INTO parcels VALUES(2, ST_GeomFromText('POLYGON ((0 0, 4 0, 4 4, 0 4, 0 0))', -1))";
        sql << "INSERT INTO parcels VALUES(3, ST_GeomFromText('POLYGON((1 1,2 1,2 2,1 2,1 1))', -1))";

        // query data in WKB form and read to geometry object
        soci::rowset<std::string> rows = (sql.prepare << "SELECT encode(ST_AsBinary(geom), 'hex') AS wkb FROM parcels");

        // calculate area of each parcel
        for (soci::rowset<std::string>::iterator it = rows.begin(); it != rows.end(); ++it)
        {
            // parse WKB and construct geometry object
            std::string const& hex = *it;
            std::vector<unsigned char> wkb;
            if (!boost::geometry::hex2wkb(*it, std::back_inserter(wkb)))
                throw std::runtime_error("hex2wkb translation failed");

            boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double> > parcel;
            if (!boost::geometry::read_wkb(wkb.begin(), wkb.end(), parcel))
                throw std::runtime_error("read_wkb failed");

            double a = boost::geometry::area(parcel);
            std::cout << "Parcel geometry: " << boost::geometry::wkt(parcel) << std::endl
                << "\thas area is " << a << " in coordinate units" << std::endl;
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return 0;
}

