// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Example showing Boost.Geometry combined with Boost.Graph, calculating shortest routes
// input: two WKT's, provided in subfolder data
// output: text, + an SVG, displayable in e.g. Firefox)

#include <iostream>
#include <fstream>
#include <iomanip>
#include <limits>

#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/io/wkt/read.hpp>


// For output:
#include <boost/geometry/io/svg/svg_mapper.hpp>

// Yes, this example currently uses an extension:

// For distance-calculations over the Earth:
//#include <boost/geometry/extensions/gis/geographic/strategies/andoyer.hpp>
// Yes, this example currently uses some extensions:


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



// Code to define properties for Boost Graph's
enum vertex_bg_property_t { vertex_bg_property };
enum edge_bg_property_t { edge_bg_property };
namespace boost
{
    BOOST_INSTALL_PROPERTY(vertex, bg_property);
    BOOST_INSTALL_PROPERTY(edge, bg_property);
}

// Define properties for vertex
template <typename Point>
struct bg_vertex_property
{
    bg_vertex_property()
    {
        boost::geometry::assign_zero(location);
    }
    bg_vertex_property(Point const& loc)
    {
        location = loc;
    }

    Point location;
};

// Define properties for edge
template <typename Linestring>
struct bg_edge_property
{
    bg_edge_property(Linestring const& line)
        : m_line(line)
    {
        m_length = boost::geometry::length(line);
    }

    inline operator double() const
    {
        return m_length;
    }

    inline Linestring const& line() const
    {
        return m_line;
    }

private :
    double m_length;
    Linestring m_line;
};

// Utility function to add a vertex to a graph. It might exist already. Then do not insert,
// but return vertex descriptor back. It might not exist. Then add it (and return).
// To efficiently handle this, a std::map is used.
template <typename M, typename K, typename G>
inline typename boost::graph_traits<G>::vertex_descriptor find_or_insert(M& map, K const& key, G& graph)
{
    typename M::const_iterator it = map.find(key);
    if (it == map.end())
    {
        // Add a vertex to the graph
        typename boost::graph_traits<G>::vertex_descriptor new_vertex
            = boost::add_vertex(graph);

        // Set the property (= location)
        boost::put(boost::get(vertex_bg_property, graph), new_vertex,
            bg_vertex_property<typename M::key_type>(key));

        // Add to the map, using POINT as key
        map[key] = new_vertex;
        return new_vertex;
    }
    return it->second;
}

template
<
    typename Graph,
    typename RoadTupleVector,
    typename CityTupleVector
>
void add_roads_and_connect_cities(Graph& graph,
            RoadTupleVector const& roads,
            CityTupleVector& cities)
{
    typedef typename boost::range_value<RoadTupleVector>::type road_type;
    typedef typename boost::tuples::element<0, road_type>::type line_type;
    typedef typename boost::geometry::point_type<line_type>::type point_type;

    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_type;

    // Define a map to be used during graph filling
    // Maps from point to vertex-id's
    typedef std::map<point_type, vertex_type, boost::geometry::less<point_type> > map_type;
    map_type map;


    // Fill the graph
    BOOST_FOREACH(road_type const& road, roads)
    {
        line_type const& line = road.template get<0>();
        // Find or add begin/end point of these line
        vertex_type from = find_or_insert(map, line.front(), graph);
        vertex_type to = find_or_insert(map, line.back(), graph);
        boost::add_edge(from, to, bg_edge_property<line_type>(line), graph);
    }

    // Find nearest graph vertex for each city, using the map
    typedef typename boost::range_value<CityTupleVector>::type city_type;
    BOOST_FOREACH(city_type& city, cities)
    {
        double min_distance = 1e300;
        for(typename map_type::const_iterator it = map.begin(); it != map.end(); ++it)
        {
            double dist = boost::geometry::distance(it->first, city.template get<0>());
            if (dist < min_distance)
            {
                min_distance = dist;
                // Set the vertex
                city.template get<2>() = it->second;
            }
        }
    }
}

template <typename Graph, typename Route>
inline void add_edge_to_route(Graph const& graph,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex1,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex2,
            Route& route)
{
    std::pair
        <
            typename boost::graph_traits<Graph>::edge_descriptor,
            bool
        > opt_edge = boost::edge(vertex1, vertex2, graph);
    if (opt_edge.second)
    {
        // Get properties of edge and of vertex
        bg_edge_property<Route> const& edge_prop =
            boost::get(boost::get(edge_bg_property, graph), opt_edge.first);

        bg_vertex_property<typename boost::geometry::point_type<Route>::type> const& vertex_prop =
            boost::get(boost::get(vertex_bg_property, graph), vertex2);

        // Depending on how edge connects to vertex, copy it forward or backward
        if (boost::geometry::equals(edge_prop.line().front(), vertex_prop.location))
        {
            std::copy(edge_prop.line().begin(), edge_prop.line().end(),
                std::back_inserter(route));
        }
        else
        {
            std::reverse_copy(edge_prop.line().begin(), edge_prop.line().end(),
                std::back_inserter(route));
        }
    }
}


template <typename Graph, typename Route>
inline void build_route(Graph const& graph,
            std::vector<typename boost::graph_traits<Graph>::vertex_descriptor> const& predecessors,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex1,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex2,
            Route& route)
{
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_type;
    vertex_type pred = predecessors[vertex2];

    add_edge_to_route(graph, vertex2, pred, route);
    while (pred != vertex1)
    {
        add_edge_to_route(graph, predecessors[pred], pred, route);
        pred = predecessors[pred];
    }
}


int main()
{
    // Define a point in the Geographic coordinate system (currently Spherical)
    // (geographic calculations are in an extension; for sample it makes no difference)
    typedef boost::geometry::model::point
        <
            double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>
        > point_type;

    typedef boost::geometry::model::linestring<point_type> line_type;

    // Define the graph, lateron containing the road network
    typedef boost::adjacency_list
        <
            boost::vecS, boost::vecS, boost::undirectedS
            , boost::property<vertex_bg_property_t, bg_vertex_property<point_type> >
            , boost::property<edge_bg_property_t, bg_edge_property<line_type> >
        > graph_type;

    typedef boost::graph_traits<graph_type>::vertex_descriptor vertex_type;


    // Init a bounding box, lateron used to define SVG map
    boost::geometry::model::box<point_type> box;
    boost::geometry::assign_inverse(box);

    // Read the cities
    typedef boost::tuple<point_type, std::string, vertex_type> city_type;
    std::vector<city_type> cities;
    read_wkt<point_type>("data/cities.wkt", cities, box);

    // Read the road network
    typedef boost::tuple<line_type, std::string> road_type;
    std::vector<road_type> roads;
    read_wkt<line_type>("data/roads.wkt", roads, box);


    graph_type graph;

    // Add roads and connect cities
    add_roads_and_connect_cities(graph, roads, cities);

    double const km = 1000.0;
    std::cout << "distances, all in KM" << std::endl
        << std::fixed << std::setprecision(0);
        
    // To calculate distance, declare and construct a strategy with average earth radius
    boost::geometry::strategy::distance::haversine<double> haversine(6372795.0);

    // Main functionality: calculate shortest routes from/to all cities
    

    // For the first one, the complete route is stored as a linestring
    bool first = true;
    line_type route;

    int const n = boost::num_vertices(graph);
    BOOST_FOREACH(city_type const& city1, cities)
    {
        std::vector<vertex_type> predecessors(n);
        std::vector<double> costs(n);

        // Call Dijkstra (without named-parameter to be compatible with all VC)
        boost::dijkstra_shortest_paths(graph, city1.get<2>(),
                &predecessors[0], &costs[0],
                boost::get(edge_bg_property, graph),
                boost::get(boost::vertex_index, graph),
                std::less<double>(), std::plus<double>(),
                (std::numeric_limits<double>::max)(), double(),
                boost::dijkstra_visitor<boost::null_visitor>());

        BOOST_FOREACH(city_type const& city2, cities)
        {
            if (! boost::equals(city1.get<1>(), city2.get<1>()))
            {
                double distance = costs[city2.get<2>()] / km;
                double acof = boost::geometry::distance(city1.get<0>(), city2.get<0>(), haversine) / km;

                std::cout
                    << std::setiosflags (std::ios_base::left) << std::setw(15)
                        << city1.get<1>() << " - "
                    << std::setiosflags (std::ios_base::left) << std::setw(15)
                        << city2.get<1>()
                    << " -> through the air: " << std::setw(4) << acof
                    << " , over the road: " << std::setw(4) << distance
                    << std::endl;

                if (first)
                {
                    build_route(graph, predecessors,
                            city1.get<2>(), city2.get<2>(),
                            route);
                    first = false;
                }
            }
        }
    }

#if defined(HAVE_SVG)
    // Create the SVG
    std::ofstream stream("routes.svg");
    boost::geometry::svg_mapper<point_type> mapper(stream, 600, 600);

    // Map roads
    BOOST_FOREACH(road_type const& road, roads)
    {
        mapper.add(road.get<0>());
    }

    BOOST_FOREACH(road_type const& road, roads)
    {
        mapper.map(road.get<0>(),
                "stroke:rgb(128,128,128);stroke-width:1");
    }

    mapper.map(route,
            "stroke:rgb(0, 255, 0);stroke-width:6;opacity:0.5");

    // Map cities
    BOOST_FOREACH(city_type const& city, cities)
    {
        mapper.map(city.get<0>(),
                "fill:rgb(255,255,0);stroke:rgb(0,0,0);stroke-width:1");
        mapper.text(city.get<0>(), city.get<1>(),
                "fill:rgb(0,0,0);font-family:Arial;font-size:10px", 5, 5);
    }
#endif    

    return 0;
}
