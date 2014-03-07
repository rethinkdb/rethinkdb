// Boost.Geometry (aka GGL, Generic Geometry Library)
// Robustness Test

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if defined(_MSC_VER)
#  pragma warning( disable : 4244 )
#  pragma warning( disable : 4267 )
#endif

#include <fstream>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/timer.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/multi/geometries/multi_geometries.hpp>

#include <boost/geometry/io/svg/svg_mapper.hpp>
#include <boost/geometry/extensions/algorithms/midpoints.hpp>

#include <boost/geometry/extensions/algorithms/buffer/buffer_inserter.hpp>

#include <boost/geometry/multi/multi.hpp> // TODO: more specific
#include <boost/geometry/extensions/algorithms/buffer/multi_buffer_inserter.hpp>

#include <boost/geometry/extensions/strategies/buffer.hpp>


#include <common/common_settings.hpp>
#include <common/make_square_polygon.hpp>


struct buffer_settings : public common_settings
{
	int join_code;
	double distance;
};

namespace bg = boost::geometry;

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename
                , Geometry1 const& mp
                , Geometry2 const& buffer
                )
{
    typedef typename boost::geometry::point_type<Geometry1>::type point_type;


    std::ofstream svg(filename.c_str());
    boost::geometry::svg_mapper<point_type> mapper(svg, 800, 800);

    boost::geometry::model::box<point_type> box;
    bg::envelope(mp, box);
    bg::buffer(box, box, 1.0);
    mapper.add(box);

    if (bg::num_points(buffer) > 0)
    {
        bg::envelope(buffer, box);
        bg::buffer(box, box, 1.0);
        mapper.add(box);
    }

    mapper.map(mp, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:3");
    mapper.map(buffer, "stroke-opacity:0.9;stroke:rgb(0,0,0);fill:none;stroke-width:1");

    //mapper.map(intersection,"opacity:0.6;stroke:rgb(0,128,0);stroke-width:5");
}




template <typename MultiPolygon, typename Settings>
bool verify(std::string const& caseid, MultiPolygon const& mp, MultiPolygon const& buffer, Settings const& settings)
{
    bool result = true;

	// Area of buffer must be larger than of original polygon
	BOOST_AUTO(area_mp, bg::area(mp));
	BOOST_AUTO(area_buf, bg::area(buffer));

	if (area_buf < area_mp)
	{
		result = false;
	}

	if (result)
	{
		typedef boost::range_value<MultiPolygon const>::type polygon_type;
		BOOST_FOREACH(polygon_type const& polygon, mp)
		{
			typename bg::point_type<polygon_type>::type point;
			bg::point_on_border(point, polygon);
			if (! bg::within(point, buffer))
			{
				result = false;
			}
		}
	}

    bool svg = settings.svg;
    bool wkt = settings.wkt;
    if (! result)
    {
        std::cout << "ERROR " << caseid << std::endl;
        //std::cout << bg::wkt(mp) << std::endl;
        //std::cout << bg::wkt(buffer) << std::endl;
        svg = true;
        wkt = true;
    }

	if (svg || wkt)
	{
		//std::cout << caseid << std::endl;
	}

    if (svg)
    {
        std::ostringstream filename;
        filename << caseid << "_"
            << typeid(typename bg::coordinate_type<MultiPolygon>::type).name()
            << ".svg";
        create_svg(filename.str(), mp, buffer);
    }

    if (wkt)
    {
        std::ostringstream filename;
        filename << caseid << "_"
            << typeid(typename bg::coordinate_type<MultiPolygon>::type).name()
            << ".wkt";
        std::ofstream stream(filename.str().c_str());
        stream << bg::wkt(mp) << std::endl;
        stream << bg::wkt(buffer) << std::endl;
    }

    return result;
}

template <typename MultiPolygon, typename Generator, typename Settings>
bool test_buffer(MultiPolygon& result, int& index,
            Generator& generator,
            int level, Settings const& settings)
{
    MultiPolygon p, q;

    // Generate two boxes
    if (level == 0)
    {
        p.resize(1);
        q.resize(1);
        make_square_polygon(p.front(), generator, settings);
        make_square_polygon(q.front(), generator, settings);
        bg::correct(p);
        bg::correct(q);
    }
    else
    {
        bg::correct(p);
        bg::correct(q);
        if (! test_buffer(p, index, generator, level - 1, settings)
            || ! test_buffer(q, index, generator, level - 1, settings))
        {
            return false;
        }
    }

    typedef typename boost::range_value<MultiPolygon>::type polygon;

    MultiPolygon mp;
    bg::detail::union_::union_insert
        <
            polygon
        >(p, q, std::back_inserter(mp));

    bg::unique(mp);
    bg::unique(mp);
    bg::correct(mp);
	result = mp;


    typedef typename bg::coordinate_type<MultiPolygon>::type coordinate_type;
    typedef typename bg::point_type<MultiPolygon>::type point_type;
    typedef bg::strategy::buffer::distance_assymetric<coordinate_type> distance_strategy_type;
    distance_strategy_type distance_strategy(settings.distance, settings.distance);

    typedef bg::strategy::buffer::join_round<point_type, point_type> join_strategy_type;
    join_strategy_type join_strategy;

    typedef typename boost::range_value<MultiPolygon>::type polygon_type;
    MultiPolygon buffered;

    std::ostringstream out;
    out << "recursive_polygons_buffer_" << index++ << "_" << level;

	try
	{
		switch(settings.join_code)
		{
			case 1 :
				bg::buffer_inserter<polygon_type>(mp, std::back_inserter(buffered),
								distance_strategy, 
								bg::strategy::buffer::join_round<point_type, point_type>());
				break;
			case 2 :
				bg::buffer_inserter<polygon_type>(mp, std::back_inserter(buffered),
								distance_strategy, 
								bg::strategy::buffer::join_miter<point_type, point_type>());
				break;
			default :
				return false;
		}
	}
    catch(std::exception const& e)
    {
		MultiPolygon empty;
		std::cout << out.str() << std::endl;
        std::cout << "Exception " << e.what() << std::endl;
	    verify(out.str(), mp, empty, settings);
		return false;
    }


    return verify(out.str(), mp, buffered, settings);
}


template <typename T, bool Clockwise, bool Closed, typename Settings>
void test_all(int seed, int count, int level, Settings const& settings)
{
    boost::timer t;

    typedef boost::minstd_rand base_generator_type;

    base_generator_type generator(seed);

    boost::uniform_int<> random_coordinate(0, settings.field_size - 1);
    boost::variate_generator<base_generator_type&, boost::uniform_int<> >
        coordinate_generator(generator, random_coordinate);

    typedef bg::model::polygon
        <
            bg::model::d2::point_xy<T>, Clockwise, Closed
        > polygon;
    typedef bg::model::multi_polygon<polygon> mp;


    int index = 0;
    for(int i = 0; i < count; i++)
    {
        mp p;
        test_buffer<mp>(p, index, coordinate_generator, level, settings);
    }
    std::cout
        << "geometries: " << index
        << " type: " << typeid(T).name()
        << " time: " << t.elapsed()  << std::endl;
}

int main(int argc, char** argv)
{
    try
    {
        namespace po = boost::program_options;
        po::options_description description("=== recursive_polygons_linear_areal ===\nAllowed options");

        int count = 1;
        int seed = static_cast<unsigned int>(std::time(0));
        int level = 3;
        bool ccw = false;
        bool open = false;
        buffer_settings settings;
        std::string form = "box";
        std::string join = "round";

        description.add_options()
            ("help", "Help message")
            ("seed", po::value<int>(&seed), "Initialization seed for random generator")
            ("count", po::value<int>(&count)->default_value(1), "Number of tests")
            ("level", po::value<int>(&level)->default_value(3), "Level to reach (higher->slower)")
            ("distance", po::value<double>(&settings.distance)->default_value(1.0), "Distance (1.0)")
            ("form", po::value<std::string>(&form)->default_value("box"), "Form of the polygons (box, triangle)")
            ("join", po::value<std::string>(&join)->default_value("round"), "Form of the joins (round, miter)")
            ("ccw", po::value<bool>(&ccw)->default_value(false), "Counter clockwise polygons")
            ("open", po::value<bool>(&open)->default_value(false), "Open polygons")
            ("size", po::value<int>(&settings.field_size)->default_value(10), "Size of the field")
            ("wkt", po::value<bool>(&settings.wkt)->default_value(false), "Create a WKT of the inputs, for all tests")
            ("svg", po::value<bool>(&settings.svg)->default_value(false), "Create a SVG for all tests")
        ;

        po::variables_map varmap;
        po::store(po::parse_command_line(argc, argv, description), varmap);
        po::notify(varmap);

        if (varmap.count("help")
            || (form != "box" && form != "triangle")
            || (join != "round" && join != "miter")
			)
        {
            std::cout << description << std::endl;
            return 1;
        }

        settings.triangular = form != "box";
		settings.join_code = join == "round" ? 1 : 2;

        if (ccw && open)
        {
            test_all<double, false, false>(seed, count, level, settings);
        }
        else if (ccw)
        {
            test_all<double, false, true>(seed, count, level, settings);
        }
        else if (open)
        {
            test_all<double, true, false>(seed, count, level, settings);
        }
        else
        {
            test_all<double, true, true>(seed, count, level, settings);
        }

#if defined(HAVE_TTMATH)
        // test_all<ttmath_big, true, true>(seed, count, max, svg, level);
#endif
    }
    catch(std::exception const& e)
    {
        std::cout << "Exception " << e.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "Other exception" << std::endl;
    }

    return 0;
}
