// Boost Graph library

//  Copyright Douglas Gregor 2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/minimal.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/subgraph.hpp>
#include <string>
#include <vector>
#include <boost/graph/adjacency_list_io.hpp>
#include <sstream>
#include <boost/graph/iteration_macros.hpp>
#include <algorithm>
#include <iterator>

using namespace std;
using namespace boost;

struct City
{
  City() {}
  City(const std::string& name, int pop, int zipcode) : name(name), population(pop)
  { 
    zipcodes.push_back(zipcode); 
  }

  string name;
  int population;
  vector<int> zipcodes;
};

std::ostream& operator<<(std::ostream& out, const City& city)
{
  out << city.name << ' ' << city.population << ' ';
  copy(city.zipcodes.begin(), city.zipcodes.end(),
       ostream_iterator<int>(out, " "));
  out << -1;
  return out;
}

std::istream& operator>>(std::istream& in, City& city)
{
  if (in >> city.name >> city.population) {
    int zip;
    city.zipcodes.clear();
    while (in >> zip && zip != -1)
      city.zipcodes.push_back(zip);
  }
  return in;
}

bool operator==(const City& c1, const City& c2)
{
  return (c1.name == c2.name && c1.population == c2.population
          && c1.zipcodes == c2.zipcodes);
}

struct Highway
{
  Highway() {}
  Highway(const string& name, double miles, int speed_limit = 65, int lanes = 4, bool divided = true) 
    : name(name), miles(miles), speed_limit(speed_limit), lanes(lanes), divided(divided) {}

  string name;
  double miles;
  int speed_limit;
  int lanes;
  bool divided;
};

std::ostream& operator<<(std::ostream& out, const Highway& highway)
{
  return out << highway.name << ' ' << highway.miles << ' ' << highway.miles
             << ' ' << highway.speed_limit << ' ' << highway.lanes
             << ' ' << highway.divided;
}

std::istream& operator>>(std::istream& in, Highway& highway)
{
  return in >> highway.name >> highway.miles >> highway.miles
            >> highway.speed_limit  >> highway.lanes
            >> highway.divided;
}

bool operator==(const Highway& h1, const Highway& h2)
{
  return (h1.name == h2.name && h1.miles == h2.miles 
          && h1.speed_limit == h2.speed_limit && h1.lanes == h2.lanes
          && h1.divided == h2.divided);
}

template<bool> struct truth {};

template<typename Map, typename VertexIterator, typename Bundle>
typename boost::graph_traits<Map>::vertex_descriptor 
do_add_vertex(Map& map, VertexIterator, const Bundle& bundle, truth<true>)
{
  return add_vertex(bundle, map);
}

template<typename Map, typename VertexIterator, typename Bundle>
typename boost::graph_traits<Map>::vertex_descriptor 
do_add_vertex(Map& map, VertexIterator& vi, const Bundle& bundle, truth<false>)
{
  get(boost::vertex_bundle, map)[*vi] = bundle;
  return *vi++;
}

template<class EL, class VL, class D, class VP, class EP, class GP>
void test_io(adjacency_list<EL,VL,D,VP,EP,GP>& map, int)
{
  typedef adjacency_list<EL,VL,D,VP,EP,GP> Map;

  ostringstream out;
  cout << write(map);
  out << write(map);
  
  istringstream in(out.str());
  adjacency_list<EL,VL,D,VP,EP,GP> map2;
  in >> read(map2);
  typename graph_traits<adjacency_list<EL,VL,D,VP,EP,GP> >::vertex_iterator
    v2 = vertices(map2).first;
  BGL_FORALL_VERTICES_T(v, map, Map) {
    BOOST_CHECK(map[v] == map2[*v2]);
    typename graph_traits<adjacency_list<EL,VL,D,VP,EP,GP> >::out_edge_iterator
      e2 = out_edges(*v2, map2).first;
    BGL_FORALL_OUTEDGES_T(v, e, map, Map) {
      BOOST_CHECK(map[e] == map[*e2]);
      ++e2;
    }
    ++v2;
  }
}

template<typename Map>
void test_io(const Map&, long)
{
  // Nothing to test
}

template<typename Map, bool CanAddVertex>
void test_bundled_properties(Map*, truth<CanAddVertex> can_add_vertex)
{
  typedef typename boost::graph_traits<Map>::vertex_iterator   vertex_iterator;
  typedef typename boost::graph_traits<Map>::vertex_descriptor vertex_descriptor;
  typedef typename boost::graph_traits<Map>::edge_descriptor   edge_descriptor;

  Map map(CanAddVertex? 2 : 3);

  vertex_iterator vi = vertices(map).first;
  vertex_descriptor v = *vi;
  map[v].name = "Troy";
  map[v].population = 49170;
  map[v].zipcodes.push_back(12180);

  ++vi;
  vertex_descriptor u = *vi++;
  map[u].name = "Albany";
  map[u].population = 95658;
  map[u].zipcodes.push_back(12201);

  // Try adding a vertex with a property value
  vertex_descriptor bloomington = do_add_vertex(map, vi, City("Bloomington", 39000, 47401),
                                                can_add_vertex);
  BOOST_CHECK(get(boost::vertex_bundle, map)[bloomington].zipcodes[0] == 47401);
  
  edge_descriptor e = add_edge(v, u, map).first;
  map[e].name = "I-87";
  map[e].miles = 10;
  map[e].speed_limit = 65;
  map[e].lanes = 4;
  map[e].divided = true;

  edge_descriptor our_trip = add_edge(v, bloomington, Highway("Long", 1000), map).first;
  BOOST_CHECK(get(boost::edge_bundle, map, our_trip).miles == 1000);
  
  BOOST_CHECK(get(get(&City::name, map), v) == "Troy");
  BOOST_CHECK(get(get(&Highway::name, map), e) == "I-87");
  BOOST_CHECK(get(&City::name, map, u) == "Albany");
  BOOST_CHECK(get(&Highway::name, map, e) == "I-87");
  put(&City::population, map, v, 49168);
  BOOST_CHECK(get(&City::population, map)[v] == 49168);
  
  boost::filtered_graph<Map, boost::keep_all> fmap(map, boost::keep_all());
  BOOST_CHECK(get(boost::edge_bundle, map, our_trip).miles == 1000);
  
  BOOST_CHECK(get(get(&City::name, fmap), v) == "Troy");
  BOOST_CHECK(get(get(&Highway::name, fmap), e) == "I-87");
  BOOST_CHECK(get(&City::name, fmap, u) == "Albany");
  BOOST_CHECK(get(&Highway::name, fmap, e) == "I-87");
  put(&City::population, fmap, v, 49169);
  BOOST_CHECK(get(&City::population, fmap)[v] == 49169);

  test_io(map, 0);
}

void test_subgraph_bundled_properties()
{
  typedef boost::subgraph<
            boost::adjacency_list<boost::vecS, boost::vecS, 
                                  boost::bidirectionalS, City, 
                                  boost::property<boost::edge_index_t, int,
                                                  Highway> > > SubMap;
  typedef boost::graph_traits<SubMap>::vertex_descriptor Vertex;
  typedef boost::graph_traits<SubMap>::vertex_iterator vertex_iterator;

  SubMap map(3);
  vertex_iterator vi = vertices(map).first;
  Vertex troy = *vi++;
  map[troy].name = "Troy";
  map[*vi++].name = "Bloomington";
  map[*vi++].name = "Endicott";

  SubMap& g1 = map.create_subgraph();
  Vertex troy1 = add_vertex(*vertices(map).first, g1);
  BOOST_CHECK(map[troy1].name == g1[troy1].name);
}

int test_main(int, char*[])
{
  typedef boost::adjacency_list<
    boost::listS, boost::vecS, boost::bidirectionalS,
    City, Highway> Map1;
  typedef boost::adjacency_matrix<boost::directedS,
    City, Highway> Map2;

  test_bundled_properties(static_cast<Map1*>(0), truth<true>());
  test_bundled_properties(static_cast<Map2*>(0), truth<false>());
  test_subgraph_bundled_properties();
  return 0;
}
