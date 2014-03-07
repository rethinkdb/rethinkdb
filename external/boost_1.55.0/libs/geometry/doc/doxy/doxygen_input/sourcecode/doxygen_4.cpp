OBSOLETE

// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Doxygen Examples, for documentation images

#include <fstream>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/multi/multi.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/io/svg/write_svg_multi.hpp>

#include <boost/geometry/io/svg/svg_mapper.hpp>


static const int wkt_countries_count = 1;
std::string wkt_countries[wkt_countries_count] = {
    "MULTIPOLYGON(((3.369472 51.37461,3.31712 51.33633,3.338228 51.27769,3.476597 51.23314,3.474252 51.2988,3.553989 51.3246,3.720502 51.27535,3.753336 51.2261,3.887015 51.21203,3.983169 51.25659,4.123883 51.28942,4.222384 51.33163,4.311502 51.38792,4.442835 51.35274,4.38655 51.46297,4.541337 51.4747,4.517883 51.40903,4.67267 51.42075,4.759444 51.48642,4.848564 51.48642,4.771171 51.3973,4.968171 51.3973,5.022111 51.48642,5.099504 51.44186,5.06667 51.36447,5.132337 51.26597,5.20973 51.33163,5.219111 51.22141,5.448946 51.26597,5.514612 51.31991,5.592005 51.27769,5.570898 51.21203,5.711612 51.18858,5.800732 51.16747,5.878123 51.13464,5.800732 51.05724,5.767898 50.93763,5.645946 50.86024,5.678778 50.76174,5.821838 50.76174,6.025336 50.75425,6.403867 50.32674,6.13802 50.1329,5.73455 49.89968,5.816617 49.54663,5.477649 49.49361,4.856486 49.79186,4.877701 50.15532,4.148557 49.97853,4.206795 50.27324,3.694881 50.31094,3.139944 50.79072,2.795442 50.72651,2.546947 51.09281,3.369472 51.37461)))"
    //"MULTIPOLYGON(((4.222384 51.33163,4.201276 51.38792,4.13561 51.36447,4.025384 51.40903,4.016003 51.48642,4.091051 51.43013,4.213003 51.42075,4.288051 51.46297,4.234109 51.4958,4.048835 51.52864,3.971442 51.58492,4.058217 51.58492,4.156717 51.58492,4.156717 51.65059,4.255217 51.61776,4.344337 51.63886,4.419384 51.6717,4.574171 51.68342,4.705504 51.70453,4.81573 51.72564,4.750063 51.73736,4.541337 51.71626,4.353717 51.72564,4.255217 51.75847,4.069943 51.83586,3.99255 51.95547,4.180169 52.04459,4.365444 52.18765,4.550717 52.45031,4.67267 52.71298,4.738337 52.94281,4.804003 52.94281,4.881396 52.86542,4.968171 52.89826,5.054944 52.90998,5.111231 52.84431,5.111231 52.77161,5.162826 52.73643,5.228492 52.74582,5.287123 52.74112,5.287123 52.69188,5.233183 52.64497,5.125301 52.61683,5.071361 52.64262,5.015075 52.61214,5.080742 52.49018,5.01742 52.44328,5.080742 52.42921,4.956444 52.35885,5.031492 52.33071,5.120612 52.31898,5.183933 52.30257,5.244909 52.30491,5.341063 52.26739,5.399694 52.23924,5.523993 52.25566,5.566207 52.3096,5.676433 52.36354,5.793696 52.41279,5.861708 52.47611,5.859363 52.53709,5.796041 52.57227,5.849981 52.59806,5.645946 52.60979,5.58966 52.64262,5.592005 52.75989,5.6436 52.80914,5.716303 52.8279,5.645946 52.84431,5.561516 52.82555,5.484123 52.84197,5.413766 52.83025,5.348099 52.87715,5.404385 52.89826,5.40673 52.9991,5.383279 53.07415,5.448946 53.21721,5.580279 53.30398,5.845291 53.36965,5.976624 53.38138,6.140792 53.39076,6.183005 53.32509,6.206459 53.39076,6.370625 53.40248,6.764627 53.47988,6.851399 53.40248,6.928792 53.33682,7.048399 53.28288,7.158627 53.25004,7.179733 53.17265,7.137519 53.12809,7.179733 52.98738,7.048399 52.87715,7.060126 52.62386,6.973351 52.63559,6.698958 52.64732,6.720066 52.56992,6.675507 52.52536,6.7529 52.47142,6.872507 52.42686,6.994459 52.48315,7.060126 52.38465,7.015567 52.29553,7.060126 52.25331,6.884233 52.13136,6.731792 52.09853,6.687233 52.02114,6.80684 52.01176,6.851399 51.94609,6.797459 51.90153,6.666125 51.90153,6.424565 51.83586,6.281507 51.85697,6.140792 51.90153,6.150171 51.83586,5.953171 51.83586,5.953171 51.74909,6.018838 51.70453,6.117339 51.6928,6.107958 51.61776,6.227565 51.51925,6.239291 51.42075,6.194732 51.34336,6.084505 51.25424,6.096231 51.1792,6.173625 51.21203,6.194732 51.14636,5.986005 51.03613,5.943792 51.08069,5.878123 51.03613,5.899231 50.97047,6.030564 50.97047,6.030564 50.9048,6.107958 50.9048,6.096231 50.83913,6.030564 50.82741,6.025336 50.75425,5.821838 50.76174,5.678778 50.76174,5.645946 50.86024,5.767898 50.93763,5.800732 51.05724,5.878123 51.13464,5.800732 51.16747,5.711612 51.18858,5.570898 51.21203,5.592005 51.27769,5.514612 51.31991,5.448946 51.26597,5.219111 51.22141,5.20973 51.33163,5.132337 51.26597,5.06667 51.36447,5.099504 51.44186,5.022111 51.48642,4.968171 51.3973,4.771171 51.3973,4.848564 51.48642,4.759444 51.48642,4.67267 51.42075,4.517883 51.40903,4.541337 51.4747,4.38655 51.46297,4.442835 51.35274,4.311502 51.38792,4.222384 51.33163)),((5.455981 52.55116,5.514612 52.5582,5.573243 52.59103,5.634219 52.59103,5.73272 52.57462,5.791351 52.56758,5.854672 52.52771,5.8406 52.46908,5.756171 52.41279,5.674088 52.39403,5.573243 52.37058,5.540409 52.31664,5.507576 52.26504,5.397349 52.25097,5.294159 52.30725,5.198003 52.33305,5.134682 52.32836,5.153444 52.39403,5.411421 52.49488,5.455981 52.55116)),((4.222384 51.33163,4.123883 51.28942,3.983169 51.25659,3.887015 51.21203,3.753336 51.2261,3.720502 51.27535,3.553989 51.3246,3.474252 51.2988,3.476597 51.23314,3.338228 51.27769,3.31712 51.33633,3.369472 51.37461,3.467216 51.41137,3.600895 51.37854,3.718157 51.34336,3.840109 51.34805,3.924538 51.36447,3.952681 51.41607,4.011312 51.39496,4.072289 51.35509,4.128574 51.32225,4.222384 51.33163)),((3.40155 51.54036,3.500049 51.58258,3.56337 51.59665,3.619656 51.57554,3.659526 51.5216,3.720502 51.51456,3.781478 51.54036,3.887015 51.54271,3.971442 51.52864,4.016003 51.50753,3.999586 51.43717,3.9433 51.46062,3.879979 51.44186,3.854181 51.38792,3.753336 51.39027,3.65249 51.46062,3.556334 51.44421,3.483632 51.48173,3.429692 51.51456,3.40155 51.54036)),((3.973788 51.84524,4.037109 51.81241,4.123883 51.7913,4.196586 51.76081,4.269289 51.71391,4.360754 51.69515,4.313848 51.65293,4.248181 51.65293,4.177824 51.67873,4.119193 51.69984,4.044145 51.71391,4.023039 51.7913,3.933919 51.80537,3.870598 51.77958,3.847145 51.83821,3.973788 51.84524)),((3.793204 51.74674,3.933919 51.73502,3.985514 51.68577,4.076979 51.667,4.116848 51.65293,4.023039 51.62714,3.931574 51.6201,3.865907 51.6459,3.826037 51.6928,3.76037 51.69984,3.692358 51.66935,3.647799 51.70922,3.713466 51.73502,3.793204 51.74674)),((4.806348 53.12574,4.879051 53.175,4.92361 53.13278,4.91423 53.0718,4.86967 53.0249,4.801658 52.99676,4.747718 52.97096,4.72192 53.0249,4.754754 53.0765,4.806348 53.12574)),((5.216766 53.39545,5.507576 53.4447,5.559171 53.43766,5.493505 53.41655,5.460672 53.39545,5.387969 53.3931,5.336373 53.37669,5.240219 53.36027,5.183933 53.33682,5.165171 53.36027,5.216766 53.39545)),((3.596204 51.60134,3.720502 51.60134,3.840109 51.61306,3.877634 51.55443,3.774442 51.56147,3.718157 51.52864,3.645454 51.56147,3.596204 51.60134)),((5.636564 53.46346,5.728029 53.44939,5.852327 53.46112,5.941446 53.45877,5.88985 53.44001,5.800732 53.43063,5.716303 53.43063,5.674088 53.41421,5.622492 53.42828,5.60373 53.45408,5.636564 53.46346)),((5.008039 53.28757,5.050254 53.30398,5.094813 53.31102,5.120612 53.28991,5.001003 53.2688,4.982243 53.24066,4.932992 53.20548,4.862634 53.19845,4.890778 53.22659,4.970516 53.2688,5.008039 53.28757)),((6.138446 53.49395,6.27447 53.50567,6.307303 53.49629,6.236946 53.46815,6.154862 53.46581,6.12672 53.44939,6.100922 53.46581,6.138446 53.49395)),((6.419876 53.54085,6.483197 53.51974,6.42691 53.51506,6.396423 53.53382,6.419876 53.54085)))",
};

static const int wkt_cities_count = 1;
std::string wkt_cities[wkt_cities_count] = {
    "MULTIPOINT(( -71.03 42.37), (-87.65 41.90), (-95.35 29.97), (-118.40 33.93), (-80.28 25.82), (-73.98 40.77), (-112.02 33.43), ( -77.04 38.85))"
};



// Read an ASCII file containing WKT's, fill a vector of tuples
// The tuples consist of at least <0> a geometry and <1> an identifying string
template <typename Geometry, typename Tuple, typename Box>
void read_wkt(std::string const& filename, std::vector<Tuple>& tuples, Box& box)
{
    std::ifstream cpp_file(filename.c_str());
    if (cpp_file.is_open())
    {
        while (! cpp_file.eof() )
        {
            std::string line;
            std::getline(cpp_file, line);
            Geometry geometry;
            boost::trim(line);
            if (! line.empty() && ! boost::starts_with(line, "#"))
            {
                std::string name;

                // Split at ';', if any
                std::string::size_type pos = line.find(";");
                if (pos != std::string::npos)
                {
                    name = line.substr(pos + 1);
                    line.erase(pos);

                    boost::trim(line);
                    boost::trim(name);
                }

                Geometry geometry;
                boost::geometry::read_wkt(line, geometry);

                Tuple tuple(geometry, name);

                tuples.push_back(tuple);
                boost::geometry::expand(box, boost::geometry::return_envelope<Box>(geometry));
            }
        }
    }
}




void svg_simplify_road()
{
    static const int n = 1;
    std::string wkt[n] = {
        "LINESTRING(-122.191 47.9758,-122.181 47.9958,-122.177 48.0022,-122.171 48.0081,-122.174 48.0402,-122.178 48.0718,-122.181 48.1036,-122.183 48.1361,-122.189 48.143,-122.206 48.205,-122.231 48.2515,-122.261 48.2977,-122.291 48.3592,-122.297 48.4234,-122.299 48.5183,-122.324 48.6237,-122.41 48.7339,-122.407 48.7538,-122.4 48.7749,-122.399 48.793,-122.423 48.8044,-122.45 48.8124,-122.481 48.8304,-122.517 48.8718,-122.521 48.8813,-122.523 48.901,-122.527 48.9105,-122.543 48.919,-122.551 48.9305,-122.561 48.9411,-122.585 48.9471,-122.612 48.9669,-122.638 48.9849,-122.661 49.0022)",
        //"LINESTRING(-122.191 47.9758,-122.204 47.9372,-122.221 47.9019,-122.242 47.8674,-122.266 47.8312)",
        //"LINESTRING(-122.176 47.5801,-122.182 47.5932,-122.185 47.6067,-122.187 47.6202,-122.187 47.6338,-122.187 47.6691,-122.182 47.7052,-122.181 47.7412,-122.192 47.776,-122.2 47.7864,-122.212 47.7945,-122.223 47.8027,-122.232 47.8132,-122.241 47.8168,-122.25 47.821,-122.259 47.8258,-122.266 47.8312)",
        //"LINESTRING(-122.193 47.5075,-122.192 47.5108,-122.192 47.5147,-122.192 47.5184,-122.192 47.5224,-122.192 47.5265,-122.192 47.5307,-122.192 47.5327,-122.191 47.5348,-122.19 47.5395,-122.189 47.5443,-122.188 47.549,-122.187 47.5538,-122.185 47.5584,-122.183 47.5609,-122.182 47.563,-122.18 47.5667,-122.179 47.5676,-122.178 47.5711,-122.177 47.5726,-122.177 47.5742,-122.177 47.5762,-122.176 47.5781,-122.176 47.5801)"
    };

    typedef boost::geometry::point_xy<double> point_type;

    std::ofstream svg("simplify_road.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 300, 300);

    boost::geometry::linestring<point_type> original[n], simplified[n];

    for (int i = 0; i < n; i++)
    {
        boost::geometry::read_wkt(wkt[i], original[i]);
        boost::geometry::simplify(original[i], simplified[i], 0.03);
        mapper.add(original[i]);
        mapper.add(simplified[i]);
        std::cout
            << "original: " << boost::size(original[i])
            << " simplified: " << boost::size(simplified[i])
            << std::endl;
    }


    for (int i = 0; i < n; i++)
    {
        mapper.map(original[i], "opacity:0.8;stroke:rgb(0,0,255);stroke-width:3");
        mapper.map(simplified[i], "opacity:0.8;stroke:rgb(0,255,0);stroke-width:2");
    }

}


void svg_simplify_country()
{

    typedef boost::geometry::point_xy<double> point_type;

    std::ofstream svg("simplify_country.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 300, 300);

    boost::geometry::multi_polygon<boost::geometry::polygon<point_type> > original[wkt_countries_count]
        , simplified[wkt_countries_count];

    for (int i = 0; i < wkt_countries_count; i++)
    {
        boost::geometry::read_wkt(wkt_countries[i], original[i]);
        boost::geometry::simplify(original[i], simplified[i], 0.1);
        mapper.add(original[i]);
        mapper.add(simplified[i]);
        std::cout
            << "original: " << boost::geometry::num_points(original[i])
            << " simplified: " << boost::geometry::num_points(simplified[i])
            << std::endl;
    }


    for (int i = 0; i < wkt_countries_count; i++)
    {
        mapper.map(original[i], "opacity:0.8;fill:none;stroke:rgb(0,0,255);stroke-width:3");
        mapper.map(simplified[i], "opacity:0.8;fill:none;stroke:rgb(0,255,0);stroke-width:2");
    }
}

void svg_convex_hull_country()
{

    typedef boost::geometry::point_xy<double> point_type;

    std::ofstream svg("convex_hull_country.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 300, 300);

    boost::geometry::multi_polygon<boost::geometry::polygon<point_type> > original[wkt_countries_count];
    boost::geometry::linear_ring<point_type> hull[wkt_countries_count];

    for (int i = 0; i < wkt_countries_count; i++)
    {
        boost::geometry::read_wkt(wkt_countries[i], original[i]);
        boost::geometry::convex_hull_inserter(original[i], std::back_inserter(hull[i]));
        mapper.add(original[i]);
        mapper.add(hull[i]);
        std::cout
            << "original: " << boost::geometry::num_points(original[i])
            << " hull: " << boost::geometry::num_points(hull[i])
            << std::endl;
    }


    for (int i = 0; i < wkt_countries_count; i++)
    {
        mapper.map(original[i], "opacity:0.8;fill:rgb(255,255,0);stroke:rgb(0,0,0);stroke-width:3");
        mapper.map(hull[i], "opacity:0.8;fill:none;stroke:rgb(255,0,0);stroke-width:3");
    }

}


void svg_convex_hull_cities()
{

    typedef boost::geometry::point_xy<double> point_type;

    std::ofstream svg("convex_hull_cities.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 300, 300);

    boost::geometry::multi_point<point_type> original[wkt_cities_count];

    boost::geometry::linear_ring<point_type> hull[wkt_cities_count];

    for (int i = 0; i < wkt_cities_count; i++)
    {
        boost::geometry::read_wkt(wkt_cities[i], original[i]);
        boost::geometry::convex_hull_inserter(original[i], std::back_inserter(hull[i]));
        mapper.add(original[i]);
        mapper.add(hull[i]);
        std::cout
            << "original: " << boost::geometry::num_points(original[i])
            << " hull: " << boost::geometry::num_points(hull[i])
            << std::endl;
    }


    for (int i = 0; i < wkt_cities_count; i++)
    {
        mapper.map(original[i], "fill:rgb(255,255,0);stroke:rgb(0,0,100);stroke-width:1", 3);
        mapper.map(hull[i], "opacity:0.8;fill:none;stroke:rgb(255,0,0);stroke-width:3");
    }
}

void svg_intersection_roads()
{
    // Read the road network
    typedef boost::geometry::point_xy<double> point_type;
    typedef boost::geometry::linestring<point_type> line_type;

    typedef boost::tuple<line_type, std::string> road_type;

    boost::geometry::box<point_type> bbox;
    boost::geometry::assign_inverse(bbox);

    std::vector<road_type> roads;
    read_wkt<line_type>("../../../example/data/roads.wkt", roads, bbox);

    // intersect
    boost::geometry::box<point_type> clip;
    std::vector<line_type> intersected;

    boost::geometry::assign(clip, -100, 25, -90, 50);
    for (size_t i = 0; i < roads.size(); i++)
    {
        boost::geometry::intersection_inserter<line_type>(clip, roads[i].get<0>(), std::back_inserter(intersected));
    }

    // create map
    std::ofstream svg("intersection_roads.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 500, 500);

    mapper.add(bbox);

    for (size_t i = 0; i < roads.size(); i++)
    {
        mapper.map(roads[i].get<0>(), "stroke:rgb(0,0,255);stroke-width:3");
    }

    for (size_t i = 0; i < intersected.size(); i++)
    {
        mapper.map(intersected[i], "stroke:rgb(0,255,0);stroke-width:2");
    }

    for (size_t i = 0; i < intersected.size(); i++)
    {
        mapper.map(clip, "fill:none;stroke:rgb(128,128,128);stroke-width:2");
    }
}


void svg_intersection_countries()
{
    // Read the road network
    typedef boost::geometry::point_xy<double> point_type;
    typedef boost::geometry::polygon<point_type> poly_type;
    typedef boost::geometry::multi_polygon<poly_type> mp_type;

    typedef boost::tuple<mp_type, std::string> country_type;

    boost::geometry::box<point_type> bbox;
    boost::geometry::assign_inverse(bbox);

    std::vector<country_type> countries;
    read_wkt<mp_type>("../../../example/data/world.wkt", countries, bbox);

    // intersect
    boost::geometry::box<point_type> clip;
    std::vector<poly_type> intersected;

    boost::geometry::assign(clip, -100, -50, 100, 50);
    for (size_t i = 0; i < countries.size(); i++)
    {
        mp_type const& mp = countries[i].get<0>();
        for (size_t j = 0; j < mp.size(); j++)
        {
            boost::geometry::intersection_inserter<poly_type>(clip, mp[j], std::back_inserter(intersected));
        }
    }

    // create map
    std::ofstream svg("intersection_countries.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 800, 800);

    mapper.add(bbox);

    for (size_t i = 0; i < countries.size(); i++)
    {
        mapper.map(countries[i].get<0>().front(), "fill:rgb(0,0,255);stroke-width:1");
    }

    for (size_t i = 0; i < intersected.size(); i++)
    {
        mapper.map(intersected[i], "fill:rgb(0,255,0);stroke-width:1");
    }

    for (size_t i = 0; i < intersected.size(); i++)
    {
        mapper.map(clip, "fill:none;stroke:rgb(128,128,128);stroke-width:2");
    }
}





int main(void)
{
    svg_intersection_roads();
    svg_intersection_countries();
    svg_simplify_road();
    svg_simplify_country();
    svg_convex_hull_country();
    svg_convex_hull_cities();
    return 0;
}
