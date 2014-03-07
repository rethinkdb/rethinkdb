// Copyright (C) 2007 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/graph/use_mpi.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/test/minimal.hpp>
#include <string>
#include <iostream>

#ifdef BOOST_NO_EXCEPTIONS
void
boost::throw_exception(std::exception const& ex)
{
    std::cout << ex.what() << std::endl;
    abort();
}
#endif

using namespace boost;
using boost::graph::distributed::mpi_process_group;

/// City structure to be attached to each vertex
struct City {
  City() {}

  City(const std::string& name, int population = -1)
    : name(name), population(population) { }
  
  template<typename Archiver>
  void serialize(Archiver& ar, const unsigned int /*version*/)
  {
    ar & name & population;
  }

  std::string name;
  int population;
};

namespace boost { namespace graph {

/// Use the City name as a key for indexing cities in a graph
template<>
struct internal_vertex_name<City>
{
  typedef multi_index::member<City, std::string, &City::name> type;
};

/// Allow the graph to build cities given only their names (filling in
/// the defaults for fields).
template<>
struct internal_vertex_constructor<City>
{
  typedef vertex_from_name<City> type;
};

} } // end namespace boost::graph

/// Our road map, where each of the vertices are cities
typedef boost::adjacency_list<vecS, distributedS<mpi_process_group, vecS>, 
                       bidirectionalS, City> RoadMap;
typedef graph_traits<RoadMap>::vertex_descriptor Vertex;

int test_main(int argc, char** argv)
{
  boost::mpi::environment env(argc, argv);

  RoadMap map;

  int rank = process_id(mpi_process_group());
  bool i_am_root = rank == 0;

  /// Create vertices for Bloomington, Indianapolis, Chicago. Everyone will
  /// try to do this, but only one of each vertex will be added.
  Vertex bloomington = add_vertex(City("Bloomington", 69291), map);
  Vertex chicago = add_vertex(City("Chicago", 9500000), map);
  Vertex indianapolis = add_vertex(City("Indianapolis", 791926), map);

  BGL_FORALL_VERTICES(city, map, RoadMap)
    std::cout << rank << ": " << map[city].name << ", population " 
              << map[city].population << std::endl;

  BOOST_CHECK(*find_vertex("Bloomington", map) == bloomington);
  BOOST_CHECK(*find_vertex("Indianapolis", map) == indianapolis);
  BOOST_CHECK(*find_vertex("Chicago", map) == chicago);

  if (i_am_root) {
    add_edge(bloomington, "Indianapolis", map);
    add_edge("Indianapolis", chicago, map);
    add_edge("Indianapolis", "Cincinnati", map);
  }

  synchronize(map);

  {
    property_map<RoadMap, std::string City::*>::type
      city_name = get(&City::name, map);

    BGL_FORALL_EDGES(road, map, RoadMap)
      std::cout << rank << ": " << get(city_name, source(road, map)) << " -> " 
                << get(city_name, target(road, map)) << std::endl;

    synchronize(map);
  }

  // Make sure the vertex for "Cincinnati" was created implicitly
  Vertex cincinnati = *find_vertex("Cincinnati", map);
  if (get(vertex_owner, map, cincinnati) 
        == process_id(map.process_group()))
    BOOST_CHECK(map[cincinnati].population == -1);

  synchronize(map);

  return 0;
}
