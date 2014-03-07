// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_GEOMETRY_REPORT_OVERLAY_ERROR
#define BOOST_GEOMETRY_NO_BOOST_TEST


#include <test_overlay_p_q.hpp>

#include <boost/program_options.hpp>
#include <boost/timer.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>


struct star_params
{
    int count; // points of ellipse, not of star
    double factor_1;
    double factor_2;
    double center_x;
    double center_y;
    double rotation;
    star_params(int c, double f1, double f2, double x, double y, double r = 0)
        : count(c)
        , factor_1(f1)
        , factor_2(f2)
        , center_x(x)
        , center_y(y)
        , rotation(r)
    {}
};



template <typename Polygon>
inline void make_star(Polygon& polygon, star_params const& p)
{
    typedef typename bg::point_type<Polygon>::type P;
    typedef typename bg::select_most_precise
        <
            typename bg::coordinate_type<Polygon>::type,
            long double
        >::type coordinate_type;

    // Create star
    coordinate_type cx = 25.0;
    coordinate_type cy = 25.0;

    coordinate_type dx = 50.0;
    coordinate_type dy = 50.0;

    coordinate_type half = 0.5;
    coordinate_type two = 2.0;

    coordinate_type a1 = coordinate_type(p.factor_1) * half * dx;
    coordinate_type b1 = coordinate_type(p.factor_1) * half * dy;
    coordinate_type a2 = coordinate_type(p.factor_2) * half * dx;
    coordinate_type b2 = coordinate_type(p.factor_2) * half * dy;

    coordinate_type pi = boost::math::constants::pi<long double>();
    coordinate_type delta = pi * two / coordinate_type(p.count - 1);
    coordinate_type angle = coordinate_type(p.rotation) * delta;
    for (int i = 0; i < p.count - 1; i++, angle += delta)
    {
        bool even = i % 2 == 0;
        coordinate_type s = sin(angle);
        coordinate_type c = cos(angle);
        coordinate_type x = p.center_x + cx + (even ? a1 : a2) * s;
        coordinate_type y = p.center_y + cy + (even ? b1 : b2) * c;
        bg::exterior_ring(polygon).push_back(bg::make<P>(x, y));

    }
    bg::exterior_ring(polygon).push_back(bg::exterior_ring(polygon).front());
    bg::correct(polygon);
}


template <typename T, bool Clockwise, bool Closed>
void test_star_ellipse(int seed, int index, star_params const& par_p,
            star_params const& par_q, p_q_settings const& settings)
{
    typedef bg::model::d2::point_xy<T> point_type;
    typedef bg::model::polygon<point_type, Clockwise, Closed> polygon;

    polygon p, q;
    make_star(p, par_p);
    make_star(q, par_q);

    std::ostringstream out;
    out << "rse_" << seed << "_" << index;
    test_overlay_p_q<polygon, T>(out.str(), p, q, settings);
}

template <typename T, bool Clockwise, bool Closed>
void test_type(int seed, int count, p_q_settings const& settings)
{
    boost::timer t;

    typedef boost::minstd_rand base_generator_type;

    //boost::uniform_real<> random_factor(0.5, 1.2);
    //boost::uniform_real<> random_location(-10.0, 10.0);
    //boost::uniform_int<> random_points(5, 20);

    // This set (next 4 lines) are now solved for the most part
    // 2009-12-03, 3 or 4 errors in 1000000 calls
    // 2009-12-07,     no errors in 1000000 calls
    //boost::uniform_real<> random_factor(1.0 - 1e-3, 1.0 + 1e-3);
    //boost::uniform_real<> random_location(-1e-3, 1e-3);
    //boost::uniform_real<> random_rotation(-1e-3, 1e-3);
    //boost::uniform_int<> random_points(3, 3);

    // 2009-12-08, still errors, see notes
    // 2009-12-09, (probably) solved by order on side
    // 2010-01-16: solved (no errors in 1000000 calls)
    //boost::uniform_real<> random_factor(1.0 - 1e-3, 1.0 + 1e-3);
    //boost::uniform_real<> random_location(-1e-3, -1e-3);
    //boost::uniform_real<> random_rotation(-1e-3, 1e-3);
    //boost::uniform_int<> random_points(3, 4);

    // This set (next 4 lines) are now solved ("distance-zero"/"merge iiii" problem)
    // 2009-12-03: 5,50 -> 2:1 000 000 wrong (2009-12-03)
    // 2010-01-16: solved (no errors in 10000000 calls)
    boost::uniform_real<> random_factor(0.3, 1.2);
    boost::uniform_real<> random_location(-20.0, +20.0); // -25.0, +25.0
    boost::uniform_real<> random_rotation(0, 0.5);
    boost::uniform_int<> random_points(5, 15);

    base_generator_type generator(seed);

    boost::variate_generator<base_generator_type&, boost::uniform_real<> >
        factor_generator(generator, random_factor);

    boost::variate_generator<base_generator_type&, boost::uniform_real<> >
        location_generator(generator, random_location);

    boost::variate_generator<base_generator_type&, boost::uniform_real<> >
        rotation_generator(generator, random_rotation);

    boost::variate_generator<base_generator_type&, boost::uniform_int<> >
        int_generator(generator, random_points);

    for(int i = 0; i < count; i++)
    {
        test_star_ellipse<T, Clockwise, Closed>(seed, i + 1,
            star_params(int_generator() * 2 + 1,
                    factor_generator(), factor_generator(),
                    location_generator(), location_generator(), rotation_generator()),
            star_params(int_generator() * 2 + 1,
                    factor_generator(), factor_generator(),
                    location_generator(), location_generator(), rotation_generator()),
            settings);
    }
    std::cout
        << "type: " << string_from_type<T>::name()
        << " time: " << t.elapsed()  << std::endl;
}

template <bool Clockwise, bool Closed>
void test_all(std::string const& type, int seed, int count, p_q_settings settings)
{
    if (type == "float")
    {
        settings.tolerance = 1.0e-3;
        test_type<float, Clockwise, Closed>(seed, count, settings);
    }
    else if (type == "double")
    {
        test_type<double, Clockwise, Closed>(seed, count, settings);
    }
#if defined(HAVE_TTMATH)
    else if (type == "ttmath")
    {
        test_type<ttmath_big, Clockwise, Closed>(seed, count, settings);
    }
#endif
}


int main(int argc, char** argv)
{
    try
    {
        namespace po = boost::program_options;
        po::options_description description("=== random_ellipses_stars ===\nAllowed options");

        int count = 1;
        int seed = static_cast<unsigned int>(std::time(0));
        std::string type = "float";
        bool ccw = false;
        bool open = false;
        p_q_settings settings;

        description.add_options()
            ("help", "Help message")
            ("seed", po::value<int>(&seed), "Initialization seed for random generator")
            ("count", po::value<int>(&count)->default_value(1), "Number of tests")
            ("diff", po::value<bool>(&settings.also_difference)->default_value(false), "Include testing on difference")
            ("ccw", po::value<bool>(&ccw)->default_value(false), "Counter clockwise polygons")
            ("open", po::value<bool>(&open)->default_value(false), "Open polygons")
            ("type", po::value<std::string>(&type)->default_value("float"), "Type (float,double)")
            ("wkt", po::value<bool>(&settings.wkt)->default_value(false), "Create a WKT of the inputs, for all tests")
            ("svg", po::value<bool>(&settings.svg)->default_value(false), "Create a SVG for all tests")
        ;

        po::variables_map varmap;
        po::store(po::parse_command_line(argc, argv, description), varmap);
        po::notify(varmap);

        if (varmap.count("help"))
        {
            std::cout << description << std::endl;
            return 1;
        }

        if (ccw && open)
        {
            test_all<false, false>(type, seed, count, settings);
        }
        else if (ccw)
        {
            test_all<false, true>(type, seed, count, settings);
        }
        else if (open)
        {
            test_all<true, false>(type, seed, count, settings);
        }
        else
        {
            test_all<true, true>(type, seed, count, settings);
        }
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
