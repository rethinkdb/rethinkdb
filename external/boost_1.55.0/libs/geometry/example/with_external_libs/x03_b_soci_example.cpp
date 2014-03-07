// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// SOCI example

// b: using WKT to retrieve points

// To build and run this example, see comments in example a

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
#include <boost/geometry/geometries/geometries.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>


struct city
{
    boost::geometry::model::point<float, 2, boost::geometry::cs::geographic<boost::geometry::degree> > location;
    std::string name;
};

namespace soci
{
    template <>
    struct type_conversion<city>
    {
        typedef soci::values base_type;

        static void from_base(const base_type& v, soci::indicator ind, city& value)
        {
            try
            {
                value.name = v.get<std::string>("name");
                boost::geometry::read_wkt(v.get<std::string>("wkt"), value.location);
            }
            catch(const std::exception& e)
            {
                std::cout << e.what() << std::endl;
            }
        }

        static void to_base(const city& value, base_type& v, soci::indicator& ind)
        {
            v.set("name", value.name);
            std::ostringstream out;
            out << boost::geometry::wkt(value.location);
            v.set("wkt", out.str());
            ind = i_ok;
        }
    };
}

int main()
{
    try
    {
        soci::session sql(soci::postgresql, "dbname=ggl user=ggl password=ggl");


        typedef std::vector<city> V;

        soci::rowset<city> rows = sql.prepare << "select name,astext(location) as wkt from cities";
        V vec;
        std::copy(rows.begin(), rows.end(), std::back_inserter(vec));

        for (V::const_iterator it = vec.begin(); it != vec.end(); ++it)
        {
            static const double sqrkm = 1000.0 * 1000.0;
            std::cout << it->name
                << "    " << boost::geometry::dsv(it->location)
                //<< "    " << boost::geometry::area(it->shape) / sqrkm << " km2"
                <<  std::endl;
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return 0;
}
