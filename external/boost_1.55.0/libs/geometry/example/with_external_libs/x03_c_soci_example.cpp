// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Boost.Geometry (aka GGL, Generic Geometry Library)
// SOCI example

// c: using WKB to retrieve geometries

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

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/extensions/gis/io/wkb/read_wkb.hpp>
#include <boost/geometry/extensions/gis/io/wkb/utility.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>

// user-defined type with GGL geometry
struct tree
{
    int id;
    boost::geometry::model::point<float, 2, boost::geometry::cs::geographic<boost::geometry::degree> > location;
};

// conversion of row of result to user-defined type - performs WKB parsing
namespace soci
{
    template <>
    struct type_conversion<tree>
    {
        typedef soci::values base_type;

        static void from_base(base_type const& v, soci::indicator ind, tree& value)
        {
            try
            {
                value.id = v.get<int>("id");

                // intermediate step: hex-encoded binary string to raw WKB
                std::string const& hex = v.get<std::string>("wkb");
                std::vector<unsigned char> wkb;
                if (!boost::geometry::hex2wkb(hex, std::back_inserter(wkb)))
                    throw std::runtime_error("hex2wkb translation failed");

                // parse WKB and construct point geometry
                if (!boost::geometry::read_wkb(wkb.begin(), wkb.end(), value.location))
                    throw std::runtime_error("read_wkb failed");
            }
            catch(const std::exception& e)
            {
                std::cout << e.what() << std::endl;
            }
        }

        static void to_base(tree const& value, base_type& v, soci::indicator& ind)
        {
            throw std::runtime_error("todo: wkb writer not yet implemented");
        }
    };
}

int main()
{
    try
    {
        // establish database connection
        soci::session sql(soci::postgresql, "dbname=ggl user=ggl password=ggl");

        // construct schema of table for trees (point geometries)
        sql << "DELETE FROM geometry_columns WHERE f_table_name = 'trees'";
        sql << "DROP TABLE IF EXISTS trees CASCADE";
        sql << "CREATE TABLE trees (id INTEGER)";
        sql << "SELECT AddGeometryColumn('trees', 'geom', -1, 'POINT', 2)";

        // insert sample data using plain WKT input
        sql << "INSERT INTO trees VALUES(1, ST_GeomFromText('POINT(1.23 2.34)', -1))";
        sql << "INSERT INTO trees VALUES(2, ST_GeomFromText('POINT(3.45 4.56)', -1))";
        sql << "INSERT INTO trees VALUES(3, ST_GeomFromText('POINT(5.67 6.78)', -1))";
        sql << "INSERT INTO trees VALUES(4, ST_GeomFromText('POINT(7.89 9.01)', -1))";

        // query data in WKB form and read to geometry object
        typedef std::vector<tree> trees_t;
        soci::rowset<tree> rows = (sql.prepare << "SELECT id, encode(ST_AsBinary(geom), 'hex') AS wkb FROM trees");
        trees_t trees;
        std::copy(rows.begin(), rows.end(), std::back_inserter(trees));

        // print trees output
        for (trees_t::const_iterator it = trees.begin(); it != trees.end(); ++it)
        {
            std::cout << "Tree #" << it->id << " located at\t" << boost::geometry::wkt(it->location) << std::endl;
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return 0;
}

