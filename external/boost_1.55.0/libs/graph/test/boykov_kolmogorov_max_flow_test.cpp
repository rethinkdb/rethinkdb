//  Copyright (c) 2006, Stephan Diederich
//
//  This code may be used under either of the following two licences:
//
//    Permission is hereby granted, free of charge, to any person
//    obtaining a copy of this software and associated documentation
//    files (the "Software"), to deal in the Software without
//    restriction, including without limitation the rights to use,
//    copy, modify, merge, publish, distribute, sublicense, and/or
//    sell copies of the Software, and to permit persons to whom the
//    Software is furnished to do so, subject to the following
//    conditions:
//
//    The above copyright notice and this permission notice shall be
//    included in all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//    OTHER DEALINGS IN THE SOFTWARE. OF SUCH DAMAGE.
//
//  Or:
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <fstream>

#include <boost/test/minimal.hpp>
#include <boost/graph/boykov_kolmogorov_max_flow.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost;

template <typename Graph, typename CapacityMap, typename ReverseEdgeMap>
std::pair< typename graph_traits<Graph>::vertex_descriptor,typename graph_traits<Graph>::vertex_descriptor>
fill_random_max_flow_graph(Graph& g, CapacityMap cap, ReverseEdgeMap rev, typename graph_traits<Graph>::vertices_size_type n_verts,
                           typename graph_traits<Graph>::edges_size_type n_edges, std::size_t seed)
{
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  const int cap_low = 1;
  const int cap_high = 1000;

  //init random numer generator
  minstd_rand gen(seed);
  //generate graph
  generate_random_graph(g, n_verts, n_edges, gen);

  //init an uniform distribution int generator
  typedef variate_generator<minstd_rand, uniform_int<int> > tIntGen;
  tIntGen int_gen(gen, uniform_int<int>(cap_low, cap_high));
  //randomize edge-capacities
  //randomize_property<edge_capacity, Graph, tIntGen> (g,int_gen); //we cannot use this, as we have no idea how properties are stored, right?
  typename graph_traits<Graph>::edge_iterator ei, e_end;
  for(boost::tie(ei,e_end) = edges(g); ei != e_end; ++ei)
    cap[*ei] = int_gen();

  //get source and sink node
  vertex_descriptor s = random_vertex(g, gen);
  vertex_descriptor t = graph_traits<Graph>::null_vertex();
  while(t == graph_traits<Graph>::null_vertex() || t == s)
    t = random_vertex(g, gen);

  //add reverse edges (ugly... how to do better?!)
  std::list<edge_descriptor> edges_copy;
  boost::tie(ei, e_end) = edges(g);
  std::copy(ei, e_end, std::back_insert_iterator< std::list<edge_descriptor> >(edges_copy));
  while(!edges_copy.empty()){
    edge_descriptor old_edge = edges_copy.front();
    edges_copy.pop_front();
    vertex_descriptor source_vertex = target(old_edge, g);
    vertex_descriptor target_vertex = source(old_edge, g);
    bool inserted;
    edge_descriptor  new_edge;
    boost::tie(new_edge,inserted) = add_edge(source_vertex, target_vertex, g);
    assert(inserted);
    rev[old_edge] = new_edge;
    rev[new_edge] = old_edge ;
    cap[new_edge] = 0;
  }
  return std::make_pair(s,t);
}

long test_adjacency_list_vecS(int n_verts, int n_edges, std::size_t seed){
  typedef adjacency_list_traits<vecS, vecS, directedS> tVectorTraits;
  typedef adjacency_list<vecS, vecS, directedS,
  property<vertex_index_t, long,
  property<vertex_predecessor_t, tVectorTraits::edge_descriptor,
  property<vertex_color_t, boost::default_color_type,
  property<vertex_distance_t, long> > > >,
  property<edge_capacity_t, long,
  property<edge_residual_capacity_t, long,
  property<edge_reverse_t, tVectorTraits::edge_descriptor > > > > tVectorGraph;

  tVectorGraph g;

  graph_traits<tVectorGraph>::vertex_descriptor src,sink;
  boost::tie(src,sink) = fill_random_max_flow_graph(g, get(edge_capacity,g), get(edge_reverse, g), n_verts, n_edges, seed);

  return boykov_kolmogorov_max_flow(g, get(edge_capacity, g),
                                    get(edge_residual_capacity, g),
                                    get(edge_reverse, g),
                                    get(vertex_predecessor, g),
                                    get(vertex_color, g),
                                    get(vertex_distance, g),
                                    get(vertex_index, g),
                                    src, sink);
}

long test_adjacency_list_listS(int n_verts, int n_edges, std::size_t seed){
  typedef adjacency_list_traits<listS, listS, directedS> tListTraits;
  typedef adjacency_list<listS, listS, directedS,
  property<vertex_index_t, long,
  property<vertex_predecessor_t, tListTraits::edge_descriptor,
  property<vertex_color_t, boost::default_color_type,
  property<vertex_distance_t, long> > > >,
  property<edge_capacity_t, long,
  property<edge_residual_capacity_t, long,
  property<edge_reverse_t, tListTraits::edge_descriptor > > > > tListGraph;

  tListGraph g;

  graph_traits<tListGraph>::vertex_descriptor src,sink;
  boost::tie(src,sink) = fill_random_max_flow_graph(g, get(edge_capacity,g), get(edge_reverse, g), n_verts, n_edges, seed);

  //initialize vertex indices
  graph_traits<tListGraph>::vertex_iterator vi,v_end;
  graph_traits<tListGraph>::vertices_size_type index = 0;
  for(boost::tie(vi, v_end) = vertices(g); vi != v_end; ++vi){
    put(vertex_index, g, *vi, index++);
  }
  return boykov_kolmogorov_max_flow(g, get(edge_capacity, g),
                                    get(edge_residual_capacity, g),
                                    get(edge_reverse, g),
                                    get(vertex_predecessor, g),
                                    get(vertex_color, g),
                                    get(vertex_distance, g),
                                    get(vertex_index, g),
                                    src, sink);
}

template<typename EdgeDescriptor>
    struct Node{
  boost::default_color_type vertex_color;
  long vertex_distance;
  EdgeDescriptor vertex_predecessor;
};

template<typename EdgeDescriptor>
    struct Link{
  long edge_capacity;
  long edge_residual_capacity;
  EdgeDescriptor edge_reverse;
};

long test_bundled_properties(int n_verts, int n_edges, std::size_t seed){
  typedef adjacency_list_traits<vecS, vecS, directedS> tTraits;
  typedef Node<tTraits::edge_descriptor> tVertex;
  typedef Link<tTraits::edge_descriptor> tEdge;
  typedef adjacency_list<vecS, vecS, directedS, tVertex, tEdge> tBundleGraph;

  tBundleGraph g;

  graph_traits<tBundleGraph>::vertex_descriptor src,sink;
  boost::tie(src,sink) = fill_random_max_flow_graph(g, get(&tEdge::edge_capacity,g), get(&tEdge::edge_reverse, g), n_verts, n_edges, seed);
  return boykov_kolmogorov_max_flow(g, get(&tEdge::edge_capacity, g),
                                    get(&tEdge::edge_residual_capacity, g),
                                    get(&tEdge::edge_reverse, g),
                                    get(&tVertex::vertex_predecessor, g),
                                    get(&tVertex::vertex_color, g),
                                    get(&tVertex::vertex_distance, g),
                                    get(vertex_index, g),
                                    src, sink);
}

long test_overloads(int n_verts, int n_edges, std::size_t seed){
  typedef adjacency_list_traits<vecS, vecS, directedS> tTraits;
  typedef property <edge_capacity_t, long,
     property<edge_residual_capacity_t, long,
     property<edge_reverse_t, tTraits::edge_descriptor> > >tEdgeProperty;
  typedef adjacency_list<vecS, vecS, directedS, no_property, tEdgeProperty> tGraph;

  tGraph g;

  graph_traits<tGraph>::vertex_descriptor src,sink;
  boost::tie(src,sink) = fill_random_max_flow_graph(g, get(edge_capacity,g), get(edge_reverse, g), n_verts, n_edges, seed);

  std::vector<graph_traits<tGraph>::edge_descriptor> predecessor_vec(n_verts);
  std::vector<default_color_type> color_vec(n_verts);
  std::vector<graph_traits<tGraph>::vertices_size_type> distance_vec(n_verts);

  long flow_overload_1 =
    boykov_kolmogorov_max_flow(g,
                               get(edge_capacity,g),
                               get(edge_residual_capacity,g),
                               get(edge_reverse,g),
                               get(vertex_index,g),
                               src, sink);

  long flow_overload_2 =
    boykov_kolmogorov_max_flow(g,
                               get(edge_capacity,g),
                               get(edge_residual_capacity,g),
                               get(edge_reverse,g),
                               boost::make_iterator_property_map(
                                 color_vec.begin(), get(vertex_index, g)),
                               get(vertex_index,g),
                               src, sink);

  BOOST_CHECK(flow_overload_1 == flow_overload_2);
  return flow_overload_1;
}

template<class Graph,
         class EdgeCapacityMap,
         class ResidualCapacityEdgeMap,
         class ReverseEdgeMap,
         class PredecessorMap,
         class ColorMap,
         class DistanceMap,
         class IndexMap>
class boykov_kolmogorov_test
  : public detail::bk_max_flow<
      Graph, EdgeCapacityMap, ResidualCapacityEdgeMap, ReverseEdgeMap,
      PredecessorMap, ColorMap, DistanceMap, IndexMap
    >
{

  typedef typename graph_traits<Graph>::edge_descriptor tEdge;
  typedef typename graph_traits<Graph>::vertex_descriptor tVertex;
  typedef typename property_traits< typename property_map<Graph, edge_capacity_t>::const_type>::value_type tEdgeVal;
  typedef typename graph_traits<Graph>::vertex_iterator tVertexIterator;
  typedef typename graph_traits<Graph>::out_edge_iterator tOutEdgeIterator;
  typedef typename property_traits<ColorMap>::value_type tColorValue;
  typedef color_traits<tColorValue> tColorTraits;
  typedef typename property_traits<DistanceMap>::value_type tDistanceVal;
  typedef typename detail::bk_max_flow<
    Graph, EdgeCapacityMap, ResidualCapacityEdgeMap, ReverseEdgeMap,
    PredecessorMap, ColorMap, DistanceMap, IndexMap
  > tSuper;
  public:
        boykov_kolmogorov_test(Graph& g,
                               typename graph_traits<Graph>::vertex_descriptor src,
                               typename graph_traits<Graph>::vertex_descriptor sink)
          : tSuper(g, get(edge_capacity,g), get(edge_residual_capacity,g),
                   get(edge_reverse, g), get(vertex_predecessor, g),
                   get(vertex_color, g), get(vertex_distance, g),
                   get(vertex_index, g), src, sink)
          { }

        void invariant_four(tVertex v) const{
          //passive nodes in S or T
          if(v == tSuper::m_source || v == tSuper::m_sink)
            return;
          typename std::list<tVertex>::const_iterator it = find(tSuper::m_orphans.begin(), tSuper::m_orphans.end(), v);
          // a node is active, if its in the active_list AND (is has_a_parent, or its already in the orphans_list or its the sink, or its the source)
          bool is_active = (tSuper::m_in_active_list_map[v] && (tSuper::has_parent(v) || it != tSuper::m_orphans.end() ));
          if(this->get_tree(v) != tColorTraits::gray() && !is_active){
            typename graph_traits<Graph>::out_edge_iterator ei,e_end;
            for(boost::tie(ei, e_end) = out_edges(v, tSuper::m_g); ei != e_end; ++ei){
              const tVertex& other_node = target(*ei, tSuper::m_g);
              if(this->get_tree(other_node) != this->get_tree(v)){
                if(this->get_tree(v) == tColorTraits::black())
                  BOOST_CHECK(tSuper::m_res_cap_map[*ei] == 0);
                else
                  BOOST_CHECK(tSuper::m_res_cap_map[tSuper::m_rev_edge_map[*ei]] == 0);
              }
             }
          }
        }

        void invariant_five(const tVertex& v) const{
          BOOST_CHECK(this->get_tree(v) != tColorTraits::gray() || tSuper::m_time_map[v] <= tSuper::m_time);
        }

        void invariant_six(const tVertex& v) const{
          if(this->get_tree(v) == tColorTraits::gray() || tSuper::m_time_map[v] != tSuper::m_time)
            return;
          else{
            tVertex current_node = v;
            tDistanceVal distance = 0;
            tColorValue color = this->get_tree(v);
            tVertex terminal = (color == tColorTraits::black()) ? tSuper::m_source : tSuper::m_sink;
            while(current_node != terminal){
              BOOST_CHECK(tSuper::has_parent(current_node));
              tEdge e = this->get_edge_to_parent(current_node);
              ++distance;
              current_node = (color == tColorTraits::black())? source(e, tSuper::m_g) : target(e, tSuper::m_g);
              if(distance > tSuper::m_dist_map[v])
                break;
            }
            BOOST_CHECK(distance == tSuper::m_dist_map[v]);
          }
        }

        void invariant_seven(const tVertex& v) const{
          if(this->get_tree(v) == tColorTraits::gray())
            return;
          else{
            tColorValue color = this->get_tree(v);
            long time = tSuper::m_time_map[v];
            tVertex current_node = v;
            while(tSuper::has_parent(current_node)){
              tEdge e = this->get_edge_to_parent(current_node);
              current_node = (color == tColorTraits::black()) ? source(e, tSuper::m_g) : target(e, tSuper::m_g);
              BOOST_CHECK(tSuper::m_time_map[current_node] >= time);
            }
          }
        }//invariant_seven

        void invariant_eight(const tVertex& v) const{
          if(this->get_tree(v) == tColorTraits::gray())
             return;
          else{
            tColorValue color = this->get_tree(v);
            long time = tSuper::m_time_map[v];
            tDistanceVal distance = tSuper::m_dist_map[v];
            tVertex current_node = v;
            while(tSuper::has_parent(current_node)){
              tEdge e = this->get_edge_to_parent(current_node);
              current_node = (color == tColorTraits::black()) ? source(e, tSuper::m_g) : target(e, tSuper::m_g);
              if(tSuper::m_time_map[current_node] == time)
                BOOST_CHECK(tSuper::m_dist_map[current_node] < distance);
            }
          }
        }//invariant_eight

        void check_invariants(){
          tVertexIterator vi, v_end;
          for(boost::tie(vi, v_end) = vertices(tSuper::m_g); vi != v_end; ++vi){
            invariant_four(*vi);
            invariant_five(*vi);
            invariant_six(*vi);
            invariant_seven(*vi);
            invariant_eight(*vi);
          }
        }

        tEdgeVal test(){
          this->add_active_node(this->m_sink);
          this->augment_direct_paths();
          check_invariants();
          //start the main-loop
          while(true){
            bool path_found;
            tEdge connecting_edge;
            boost::tie(connecting_edge, path_found) = this->grow(); //find a path from source to sink
            if(!path_found){
                //we're finished, no more paths were found
              break;
            }
            check_invariants();
            this->m_time++;
            this->augment(connecting_edge); //augment that path
            check_invariants();
            this->adopt(); //rebuild search tree structure
            check_invariants();
          }

          //check if flow is the sum of outgoing edges of src
          tOutEdgeIterator ei, e_end;
          tEdgeVal src_sum = 0;
          for(boost::tie(ei, e_end) = out_edges(this->m_source, this->m_g); ei != e_end; ++ei){
            src_sum += this->m_cap_map[*ei] - this->m_res_cap_map[*ei];
          }
          BOOST_CHECK(this->m_flow == src_sum);
          //check if flow is the sum of ingoing edges of sink
          tEdgeVal sink_sum = 0;
          for(boost::tie(ei, e_end) = out_edges(this->m_sink, this->m_g); ei != e_end; ++ei){
            tEdge in_edge = this->m_rev_edge_map[*ei];
            sink_sum += this->m_cap_map[in_edge] - this->m_res_cap_map[in_edge];
          }
          BOOST_CHECK(this->m_flow == sink_sum);
          return this->m_flow;
        }
};

long test_algorithms_invariant(int n_verts, int n_edges, std::size_t seed)
{
  typedef adjacency_list_traits<vecS, vecS, directedS> tVectorTraits;
  typedef adjacency_list<vecS, vecS, directedS,
  property<vertex_index_t, long,
  property<vertex_predecessor_t, tVectorTraits::edge_descriptor,
  property<vertex_color_t, default_color_type,
  property<vertex_distance_t, long> > > >,
  property<edge_capacity_t, long,
  property<edge_residual_capacity_t, long,
  property<edge_reverse_t, tVectorTraits::edge_descriptor > > > > tVectorGraph;

  tVectorGraph g;

  graph_traits<tVectorGraph>::vertex_descriptor src, sink;
  boost::tie(src,sink) = fill_random_max_flow_graph(g, get(edge_capacity,g), get(edge_reverse, g), n_verts, n_edges, seed);

  typedef property_map<tVectorGraph, edge_capacity_t>::type tEdgeCapMap;
  typedef property_map<tVectorGraph, edge_residual_capacity_t>::type tEdgeResCapMap;
  typedef property_map<tVectorGraph, edge_reverse_t>::type tRevEdgeMap;
  typedef property_map<tVectorGraph, vertex_predecessor_t>::type tVertexPredMap;
  typedef property_map<tVectorGraph, vertex_color_t>::type tVertexColorMap;
  typedef property_map<tVectorGraph, vertex_distance_t>::type tDistanceMap;
  typedef property_map<tVectorGraph, vertex_index_t>::type tIndexMap;
  typedef boykov_kolmogorov_test<
    tVectorGraph, tEdgeCapMap, tEdgeResCapMap, tRevEdgeMap, tVertexPredMap,
    tVertexColorMap, tDistanceMap, tIndexMap
  > tKolmo;
  tKolmo instance(g, src, sink);
  return instance.test();
}

int test_main(int argc, char* argv[])
{
  int n_verts = 10;
  int n_edges = 500;
  std::size_t seed = 1;

  if (argc > 1) n_verts = lexical_cast<int>(argv[1]);
  if (argc > 2) n_edges = lexical_cast<int>(argv[2]);
  if (argc > 3) seed = lexical_cast<std::size_t>(argv[3]);

  //we need at least 2 vertices to create src and sink in random graphs
  //this case is also caught in boykov_kolmogorov_max_flow
  if (n_verts<2)
    n_verts = 2;

  // below are checks for different calls to boykov_kolmogorov_max_flow and different graph-types

  //checks support of vecS storage
  long flow_vecS = test_adjacency_list_vecS(n_verts, n_edges, seed);
  std::cout << "vecS flow: " << flow_vecS << std::endl;
  //checks support of listS storage (especially problems with vertex indices)
  long flow_listS = test_adjacency_list_listS(n_verts, n_edges, seed);
  std::cout << "listS flow: " << flow_listS << std::endl;
  BOOST_CHECK(flow_vecS == flow_listS);
  //checks bundled properties
  long flow_bundles = test_bundled_properties(n_verts, n_edges, seed);
  std::cout << "bundles flow: " << flow_bundles << std::endl;
  BOOST_CHECK(flow_listS == flow_bundles);
  //checks overloads
  long flow_overloads = test_overloads(n_verts, n_edges, seed);
  std::cout << "overloads flow: " << flow_overloads << std::endl;
  BOOST_CHECK(flow_bundles == flow_overloads);

  // excessive test version where Boykov-Kolmogorov's algorithm invariants are
  // checked
  long flow_invariants = test_algorithms_invariant(n_verts, n_edges, seed);
  std::cout << "invariants flow: " << flow_invariants << std::endl;
  BOOST_CHECK(flow_overloads == flow_invariants);
  return 0;
}
