//=======================================================================
// Copyright (c) 2005 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//=======================================================================

#include <boost/graph/max_cardinality_matching.hpp>

#include <iostream>                      // for std::cout
#include <boost/property_map/vector_property_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/random.hpp>
#include <ctime>
#include <boost/random.hpp>
#include <boost/test/minimal.hpp>

using namespace boost;

typedef adjacency_list<vecS, 
                       vecS, 
                       undirectedS, 
                       property<vertex_index_t, int> >  undirected_graph;

typedef adjacency_list<listS,
                       listS,
                       undirectedS,
                       property<vertex_index_t, int> >  undirected_list_graph;

typedef adjacency_matrix<undirectedS, 
                         property<vertex_index_t,int> > undirected_adjacency_matrix_graph;


template <typename Graph>
struct vertex_index_installer 
{
  static void install(Graph&) {}
};


template <>
struct vertex_index_installer<undirected_list_graph>
{
  static void install(undirected_list_graph& g) 
  {
    typedef graph_traits<undirected_list_graph>::vertex_iterator vertex_iterator_t;
    typedef graph_traits<undirected_list_graph>::vertices_size_type v_size_t;
    
    vertex_iterator_t vi, vi_end;
    v_size_t i = 0;
    for(boost::tie(vi,vi_end) = vertices(g); vi != vi_end; ++vi, ++i)
      put(vertex_index, g, *vi, i);
  }
};



template <typename Graph>
void complete_graph(Graph& g, int n)
{
  //creates the complete graph on n vertices
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator_t;

  g = Graph(n);
  vertex_iterator_t vi, vi_end, wi;
  boost::tie(vi,vi_end) = vertices(g);
  for(boost::tie(vi,vi_end) = vertices(g); vi != vi_end; ++vi)
    {
      wi = vi;
      ++wi;
      for(; wi != vi_end; ++wi)
        add_edge(*vi,*wi,g);      
    }
}



template <typename Graph>
void gabows_graph(Graph& g, int n)
{
  //creates a graph with 2n vertices, consisting of the complete graph
  //on n vertices plus n vertices of degree one, each adjacent to one
  //vertex in the complete graph. without any initial matching, this 
  //graph forces edmonds' algorithm into worst-case behavior.

  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator_t;

  g = Graph(2* n);

  vertex_iterator_t vi, vi_end, ui, ui_end, halfway;

  boost::tie(ui,ui_end) = vertices(g);

  halfway = ui;
  for(int i = 0; i < n; ++i)
    ++halfway;


  while(ui != halfway)
    {
      vi = ui;
      ++vi;
      while (vi != halfway)
        {
          add_edge(*ui,*vi,g);
          ++vi;
        }
      ++ui;
    }

  boost::tie(ui,ui_end) = vertices(g);

  while(halfway != ui_end)
    {
      add_edge(*ui, *halfway, g);
      ++ui;
      ++halfway;
    }

}



template <typename Graph>
void matching_test(std::size_t num_v, const std::string& graph_name)
{
  typedef typename property_map<Graph,vertex_index_t>::type vertex_index_map_t;
  typedef vector_property_map< typename graph_traits<Graph>::vertex_descriptor, vertex_index_map_t > mate_t;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;

  const std::size_t double_num_v = num_v * 2;

  bool all_tests_passed = true;

  //form the complete graph on 2*n vertices; the maximum cardinality matching, greedy matching,
  //and extra greedy matching should all be the same - a matching of size n.

  Graph g(double_num_v);
  complete_graph(g,double_num_v);

  vertex_index_installer<Graph>::install(g);

  mate_t edmonds_mate(double_num_v);
  mate_t greedy_mate(double_num_v);
  mate_t extra_greedy_mate(double_num_v);
  
  //find a maximum cardinality matching using edmonds' blossom-shrinking algorithm, starting
  //with an empty matching.
  bool edmonds_result =
    matching < Graph, mate_t, vertex_index_map_t,
               edmonds_augmenting_path_finder, empty_matching, maximum_cardinality_matching_verifier>
    (g,edmonds_mate, get(vertex_index,g));

  BOOST_CHECK (edmonds_result);
  if (!edmonds_result)
    {
      std::cout << "Verifier reporting a problem finding a perfect matching on" << std::endl
                << "the complete graph using " << graph_name << std::endl;
      all_tests_passed = false;
    }
  
  //find a greedy matching
  bool greedy_result =
    matching<Graph, mate_t, vertex_index_map_t,
             no_augmenting_path_finder, greedy_matching, maximum_cardinality_matching_verifier>
    (g,greedy_mate, get(vertex_index,g));

  BOOST_CHECK (greedy_result);
  if (!greedy_result)
    {
      std::cout << "Verifier reporting a problem finding a greedy matching on" << std::endl
                << "the complete graph using " << graph_name << std::endl;
      all_tests_passed = false;
    }

  //find an extra greedy matching
  bool extra_greedy_result =
    matching<Graph, mate_t, vertex_index_map_t,
             no_augmenting_path_finder, extra_greedy_matching, maximum_cardinality_matching_verifier>
    (g,extra_greedy_mate,get(vertex_index,g));

  BOOST_CHECK (extra_greedy_result);
  if (!extra_greedy_result)
    {
      std::cout << "Verifier reporting a problem finding an extra greedy matching on" << std::endl
                << "the complete graph using " << graph_name << std::endl;
      all_tests_passed = false;
    }

  //as a sanity check, make sure that each of the matchings returned is a valid matching and has
  //1000 edges.

  bool edmonds_sanity_check = 
    is_a_matching(g,edmonds_mate) && matching_size(g,edmonds_mate) == num_v;
  
  BOOST_CHECK (edmonds_sanity_check);
  if (edmonds_result && !edmonds_sanity_check)
    {
      std::cout << "Verifier okayed edmonds' algorithm on the complete graph, but" << std::endl
                << "the matching returned either wasn't a valid matching or wasn't" << std::endl
                << "actually a maximum cardinality matching." << std::endl;
      all_tests_passed = false;
    }

  bool greedy_sanity_check = 
    is_a_matching(g,greedy_mate) && matching_size(g,greedy_mate) == num_v;  

  BOOST_CHECK (greedy_sanity_check);
  if (greedy_result && !greedy_sanity_check)
    {
      std::cout << "Verifier okayed greedy algorithm on the complete graph, but" << std::endl
                << "the matching returned either wasn't a valid matching or wasn't" << std::endl
                << "actually a maximum cardinality matching." << std::endl;
      all_tests_passed = false;
    }
  
  bool extra_greedy_sanity_check =
    is_a_matching(g,extra_greedy_mate) && matching_size(g,extra_greedy_mate) == num_v;
  
  BOOST_CHECK (extra_greedy_sanity_check);
  if (extra_greedy_result && !extra_greedy_sanity_check)
    {
      std::cout << "Verifier okayed extra greedy algorithm on the complete graph, but" << std::endl
                << "the matching returned either wasn't a valid matching or wasn't" << std::endl
                << "actually a maximum cardinality matching." << std::endl;
      all_tests_passed = false;
    }
  
  //Now remove an edge from the edmonds_mate matching.
  vertex_iterator_t vi,vi_end;
  for(boost::tie(vi,vi_end) = vertices(g); vi != vi_end; ++vi)
    if (edmonds_mate[*vi] != graph_traits<Graph>::null_vertex())
      break;
  
  edmonds_mate[edmonds_mate[*vi]] = graph_traits<Graph>::null_vertex();
  edmonds_mate[*vi] = graph_traits<Graph>::null_vertex();
  
  //...and run the matching verifier - it should tell us that the matching isn't
  //a maximum matching.
  bool modified_edmonds_verification_result =
    maximum_cardinality_matching_verifier<Graph,mate_t,vertex_index_map_t>::verify_matching(g,edmonds_mate,get(vertex_index,g));
  
  BOOST_CHECK (!modified_edmonds_verification_result);
  if (modified_edmonds_verification_result)
    {
      std::cout << "Verifier was fooled into thinking that a non-maximum matching was maximum" << std::endl;
      all_tests_passed = false;
    }
  
  Graph h(double_num_v);
  gabows_graph(h,num_v);

  vertex_index_installer<Graph>::install(h);

  //gabow's graph always has a perfect matching. it's also a good example of why
  //one should initialize with the extra_greedy_matching in most cases.
  
  mate_t gabow_mate(double_num_v);
  
  bool gabows_graph_result = checked_edmonds_maximum_cardinality_matching(h,gabow_mate);
  
  BOOST_CHECK (gabows_graph_result);
  if (!gabows_graph_result)
    {
      std::cout << "Problem on Gabow's Graph with " << graph_name << ":" << std::endl
                << "   Verifier reporting a maximum cardinality matching not found." << std::endl;
      all_tests_passed = false;
    }
  
  BOOST_CHECK (matching_size(h,gabow_mate) == num_v);
  if (gabows_graph_result && matching_size(h,gabow_mate) != num_v)
    {
      std::cout << "Problem on Gabow's Graph with " << graph_name << ":" << std::endl
                << "   Verifier reported a maximum cardinality matching found," << std::endl
                << "   but matching size is " << matching_size(h,gabow_mate)
                << " when it should be " << num_v << std::endl;
      all_tests_passed = false;
    }

  Graph j(double_num_v);
  vertex_index_installer<Graph>::install(j);

  typedef boost::mt19937 base_generator_type;
  base_generator_type generator(static_cast<unsigned int>(std::time(0)));
  boost::uniform_int<> distribution(0, double_num_v-1);
  boost::variate_generator<base_generator_type&, boost::uniform_int<> > rand_num(generator, distribution);

  std::size_t num_edges = 0;
  bool success;

  while (num_edges < 4*double_num_v)
    {
      vertex_descriptor_t u = random_vertex(j,rand_num);
      vertex_descriptor_t v = random_vertex(j,rand_num);
      if (u != v)
        {
          boost::tie(tuples::ignore, success) = add_edge(u, v, j);
          if (success)
            num_edges++;
        }
    }

  mate_t random_mate(double_num_v);
  bool random_graph_result = checked_edmonds_maximum_cardinality_matching(j,random_mate);  

  BOOST_CHECK(random_graph_result);
  if (!random_graph_result)
    {
      std::cout << "Matching in random graph not maximum for " << graph_name << std::endl;
      all_tests_passed = false;
    }

  //Now remove an edge from the random_mate matching.
  for(boost::tie(vi,vi_end) = vertices(j); vi != vi_end; ++vi)
    if (random_mate[*vi] != graph_traits<Graph>::null_vertex())
      break;
  
  random_mate[random_mate[*vi]] = graph_traits<Graph>::null_vertex();
  random_mate[*vi] = graph_traits<Graph>::null_vertex();
  
  //...and run the matching verifier - it should tell us that the matching isn't
  //a maximum matching.
  bool modified_random_verification_result =
    maximum_cardinality_matching_verifier<Graph,mate_t,vertex_index_map_t>::verify_matching(j,random_mate,get(vertex_index,j));
  
  BOOST_CHECK(!modified_random_verification_result);
  if (modified_random_verification_result)
    {
      std::cout << "Verifier was fooled into thinking that a non-maximum matching was maximum" << std::endl;
      all_tests_passed = false;
    }

  BOOST_CHECK(all_tests_passed);
  if (all_tests_passed)
    {
      std::cout << graph_name << " passed all tests for n = " << num_v << '.' << std::endl;
    }

}




int test_main(int, char*[])
{

  matching_test<undirected_graph>(10, "adjacency_list (using vectors)");
  matching_test<undirected_list_graph>(10, "adjacency_list (using lists)");
  matching_test<undirected_adjacency_matrix_graph>(10, "adjacency_matrix");

  matching_test<undirected_graph>(20, "adjacency_list (using vectors)");
  matching_test<undirected_list_graph>(20, "adjacency_list (using lists)");
  matching_test<undirected_adjacency_matrix_graph>(20, "adjacency_matrix");

  matching_test<undirected_graph>(21, "adjacency_list (using vectors)");
  matching_test<undirected_list_graph>(21, "adjacency_list (using lists)");
  matching_test<undirected_adjacency_matrix_graph>(21, "adjacency_matrix");

#if 0
  matching_test<undirected_graph>(50, "adjacency_list (using vectors)");
  matching_test<undirected_list_graph>(50, "adjacency_list (using lists)");
  matching_test<undirected_adjacency_matrix_graph>(50, "adjacency_matrix");

  matching_test<undirected_graph>(51, "adjacency_list (using vectors)");
  matching_test<undirected_list_graph>(51, "adjacency_list (using lists)");
  matching_test<undirected_adjacency_matrix_graph>(51, "adjacency_matrix");
#endif

  return 0;
}


