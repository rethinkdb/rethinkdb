/* adjacency_matrix_test.cpp source file
 *
 * Copyright Cromwell D. Enage 2004
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * Defines the std::ios class and std::cout, its global output instance.
 */
#include <iostream>

/*
 * Defines the boost::property_map class template and the boost::get and
 * boost::put function templates.
 */
#include <boost/property_map/property_map.hpp>

/*
 * Defines the boost::graph_traits class template.
 */
#include <boost/graph/graph_traits.hpp>

/*
 * Defines the vertex and edge property tags.
 */
#include <boost/graph/properties.hpp>

/*
 * Defines the boost::adjacency_list class template and its associated
 * non-member function templates.
 */
#include <boost/graph/adjacency_list.hpp>

/*
 * Defines the boost::adjacency_matrix class template and its associated
 * non-member function templates.
 */
#include <boost/graph/adjacency_matrix.hpp>

#include <boost/test/minimal.hpp>

#include <vector>
#include <algorithm> // For std::sort
#include <boost/type_traits/is_convertible.hpp>

#include <boost/graph/iteration_macros.hpp>

template<typename Graph1, typename Graph2>
void run_test()
{
   typedef typename boost::property_map<Graph1, boost::vertex_index_t>::type
           IndexMap1;
   typedef typename boost::property_map<Graph2, boost::vertex_index_t>::type
           IndexMap2;

   Graph1 g1(24);
   Graph2 g2(24);

boost::add_edge(boost::vertex(1, g1), boost::vertex(2, g1), g1);
   boost::add_edge(boost::vertex(1, g2), boost::vertex(2, g2), g2);
   boost::add_edge(boost::vertex(2, g1), boost::vertex(10, g1), g1);
   boost::add_edge(boost::vertex(2, g2), boost::vertex(10, g2), g2);
   boost::add_edge(boost::vertex(2, g1), boost::vertex(5, g1), g1);
   boost::add_edge(boost::vertex(2, g2), boost::vertex(5, g2), g2);
   boost::add_edge(boost::vertex(3, g1), boost::vertex(10, g1), g1);
   boost::add_edge(boost::vertex(3, g2), boost::vertex(10, g2), g2);
   boost::add_edge(boost::vertex(3, g1), boost::vertex(0, g1), g1);
   boost::add_edge(boost::vertex(3, g2), boost::vertex(0, g2), g2);
   boost::add_edge(boost::vertex(4, g1), boost::vertex(5, g1), g1);
   boost::add_edge(boost::vertex(4, g2), boost::vertex(5, g2), g2);
   boost::add_edge(boost::vertex(4, g1), boost::vertex(0, g1), g1);
   boost::add_edge(boost::vertex(4, g2), boost::vertex(0, g2), g2);
   boost::add_edge(boost::vertex(5, g1), boost::vertex(14, g1), g1);
   boost::add_edge(boost::vertex(5, g2), boost::vertex(14, g2), g2);
   boost::add_edge(boost::vertex(6, g1), boost::vertex(3, g1), g1);
   boost::add_edge(boost::vertex(6, g2), boost::vertex(3, g2), g2);
   boost::add_edge(boost::vertex(7, g1), boost::vertex(17, g1), g1);
   boost::add_edge(boost::vertex(7, g2), boost::vertex(17, g2), g2);
   boost::add_edge(boost::vertex(7, g1), boost::vertex(11, g1), g1);
   boost::add_edge(boost::vertex(7, g2), boost::vertex(11, g2), g2);
   boost::add_edge(boost::vertex(8, g1), boost::vertex(17, g1), g1);
   boost::add_edge(boost::vertex(8, g2), boost::vertex(17, g2), g2);
   boost::add_edge(boost::vertex(8, g1), boost::vertex(1, g1), g1);
   boost::add_edge(boost::vertex(8, g2), boost::vertex(1, g2), g2);
   boost::add_edge(boost::vertex(9, g1), boost::vertex(11, g1), g1);
   boost::add_edge(boost::vertex(9, g2), boost::vertex(11, g2), g2);
   boost::add_edge(boost::vertex(9, g1), boost::vertex(1, g1), g1);
   boost::add_edge(boost::vertex(9, g2), boost::vertex(1, g2), g2);
   boost::add_edge(boost::vertex(10, g1), boost::vertex(19, g1), g1);
   boost::add_edge(boost::vertex(10, g2), boost::vertex(19, g2), g2);
   boost::add_edge(boost::vertex(10, g1), boost::vertex(15, g1), g1);
   boost::add_edge(boost::vertex(10, g2), boost::vertex(15, g2), g2);
   boost::add_edge(boost::vertex(10, g1), boost::vertex(8, g1), g1);
   boost::add_edge(boost::vertex(10, g2), boost::vertex(8, g2), g2);
   boost::add_edge(boost::vertex(11, g1), boost::vertex(19, g1), g1);
   boost::add_edge(boost::vertex(11, g2), boost::vertex(19, g2), g2);
   boost::add_edge(boost::vertex(11, g1), boost::vertex(15, g1), g1);
   boost::add_edge(boost::vertex(11, g2), boost::vertex(15, g2), g2);
   boost::add_edge(boost::vertex(11, g1), boost::vertex(4, g1), g1);
   boost::add_edge(boost::vertex(11, g2), boost::vertex(4, g2), g2);
   boost::add_edge(boost::vertex(12, g1), boost::vertex(19, g1), g1);
   boost::add_edge(boost::vertex(12, g2), boost::vertex(19, g2), g2);
   boost::add_edge(boost::vertex(12, g1), boost::vertex(8, g1), g1);
   boost::add_edge(boost::vertex(12, g2), boost::vertex(8, g2), g2);
   boost::add_edge(boost::vertex(12, g1), boost::vertex(4, g1), g1);
   boost::add_edge(boost::vertex(12, g2), boost::vertex(4, g2), g2);
   boost::add_edge(boost::vertex(13, g1), boost::vertex(15, g1), g1);
   boost::add_edge(boost::vertex(13, g2), boost::vertex(15, g2), g2);
   boost::add_edge(boost::vertex(13, g1), boost::vertex(8, g1), g1);
   boost::add_edge(boost::vertex(13, g2), boost::vertex(8, g2), g2);
   boost::add_edge(boost::vertex(13, g1), boost::vertex(4, g1), g1);
   boost::add_edge(boost::vertex(13, g2), boost::vertex(4, g2), g2);
   boost::add_edge(boost::vertex(14, g1), boost::vertex(22, g1), g1);
   boost::add_edge(boost::vertex(14, g2), boost::vertex(22, g2), g2);
   boost::add_edge(boost::vertex(14, g1), boost::vertex(12, g1), g1);
   boost::add_edge(boost::vertex(14, g2), boost::vertex(12, g2), g2);
   boost::add_edge(boost::vertex(15, g1), boost::vertex(22, g1), g1);
   boost::add_edge(boost::vertex(15, g2), boost::vertex(22, g2), g2);
   boost::add_edge(boost::vertex(15, g1), boost::vertex(6, g1), g1);
   boost::add_edge(boost::vertex(15, g2), boost::vertex(6, g2), g2);
   boost::add_edge(boost::vertex(16, g1), boost::vertex(12, g1), g1);
   boost::add_edge(boost::vertex(16, g2), boost::vertex(12, g2), g2);
   boost::add_edge(boost::vertex(16, g1), boost::vertex(6, g1), g1);
   boost::add_edge(boost::vertex(16, g2), boost::vertex(6, g2), g2);
   boost::add_edge(boost::vertex(17, g1), boost::vertex(20, g1), g1);
   boost::add_edge(boost::vertex(17, g2), boost::vertex(20, g2), g2);
   boost::add_edge(boost::vertex(18, g1), boost::vertex(9, g1), g1);
   boost::add_edge(boost::vertex(18, g2), boost::vertex(9, g2), g2);
   boost::add_edge(boost::vertex(19, g1), boost::vertex(23, g1), g1);
   boost::add_edge(boost::vertex(19, g2), boost::vertex(23, g2), g2);
   boost::add_edge(boost::vertex(19, g1), boost::vertex(18, g1), g1);
   boost::add_edge(boost::vertex(19, g2), boost::vertex(18, g2), g2);
   boost::add_edge(boost::vertex(20, g1), boost::vertex(23, g1), g1);
   boost::add_edge(boost::vertex(20, g2), boost::vertex(23, g2), g2);
   boost::add_edge(boost::vertex(20, g1), boost::vertex(13, g1), g1);
   boost::add_edge(boost::vertex(20, g2), boost::vertex(13, g2), g2);
   boost::add_edge(boost::vertex(21, g1), boost::vertex(18, g1), g1);
   boost::add_edge(boost::vertex(21, g2), boost::vertex(18, g2), g2);
   boost::add_edge(boost::vertex(21, g1), boost::vertex(13, g1), g1);
   boost::add_edge(boost::vertex(21, g2), boost::vertex(13, g2), g2);
   boost::add_edge(boost::vertex(22, g1), boost::vertex(21, g1), g1);
   boost::add_edge(boost::vertex(22, g2), boost::vertex(21, g2), g2);
   boost::add_edge(boost::vertex(23, g1), boost::vertex(16, g1), g1);
   boost::add_edge(boost::vertex(23, g2), boost::vertex(16, g2), g2);

   IndexMap1 index_map1 = boost::get(boost::vertex_index_t(), g1);
   IndexMap2 index_map2 = boost::get(boost::vertex_index_t(), g2);
   typename boost::graph_traits<Graph1>::vertex_iterator vi1, vend1;
   typename boost::graph_traits<Graph2>::vertex_iterator vi2, vend2;

   typename boost::graph_traits<Graph1>::adjacency_iterator ai1, aend1;
   typename boost::graph_traits<Graph2>::adjacency_iterator ai2, aend2;

   for (boost::tie(vi1, vend1) = boost::vertices(g1), boost::tie(vi2, vend2) =boost::vertices(g2); vi1 != vend1; ++vi1, ++vi2)
   {
      BOOST_CHECK(boost::get(index_map1, *vi1) == boost::get(index_map2, *vi2));

      for (boost::tie(ai1, aend1) = boost::adjacent_vertices(*vi1, g1),
             boost::tie(ai2, aend2) = boost::adjacent_vertices(*vi2, g2);
           ai1 != aend1;
           ++ai1, ++ai2)
      {
        BOOST_CHECK(boost::get(index_map1, *ai1) == boost::get(index_map2, *ai2));
      }
   }

   typename boost::graph_traits<Graph1>::out_edge_iterator ei1, eend1;
   typename boost::graph_traits<Graph2>::out_edge_iterator ei2, eend2;

   for (boost::tie(vi1, vend1) = boost::vertices(g1),
          boost::tie(vi2, vend2) = boost::vertices(g2); vi1 != vend1; ++vi1, ++vi2)
   {
      BOOST_CHECK(boost::get(index_map1, *vi1) == boost::get(index_map2, *vi2));

      for (boost::tie(ei1, eend1) = boost::out_edges(*vi1, g1),
             boost::tie(ei2, eend2) = boost::out_edges(*vi2, g2);
           ei1 != eend1;
           ++ei1, ++ei2)
      {
        BOOST_CHECK(boost::get(index_map1, boost::target(*ei1, g1)) == boost::get(index_map2, boost::target(*ei2, g2)));
      }
   }

   typename boost::graph_traits<Graph1>::in_edge_iterator iei1, ieend1;
   typename boost::graph_traits<Graph2>::in_edge_iterator iei2, ieend2;

   for (boost::tie(vi1, vend1) = boost::vertices(g1),
          boost::tie(vi2, vend2) = boost::vertices(g2); vi1 != vend1; ++vi1, ++vi2)
   {
      BOOST_CHECK(boost::get(index_map1, *vi1) == boost::get(index_map2, *vi2));

      for (boost::tie(iei1, ieend1) = boost::in_edges(*vi1, g1),
             boost::tie(iei2, ieend2) = boost::in_edges(*vi2, g2);
           iei1 != ieend1;
           ++iei1, ++iei2)
      {
        BOOST_CHECK(boost::get(index_map1, boost::target(*iei1, g1)) == boost::get(index_map2, boost::target(*iei2, g2)));
      }
   }

   // Test construction from a range of pairs
   std::vector<std::pair<int, int> > edge_pairs_g1;
   BGL_FORALL_EDGES_T(e, g1, Graph1) {
     edge_pairs_g1.push_back(
       std::make_pair(get(index_map1, source(e, g1)),
                      get(index_map1, target(e, g1))));
   }
   Graph2 g3(edge_pairs_g1.begin(), edge_pairs_g1.end(), num_vertices(g1));
   BOOST_CHECK(num_vertices(g1) == num_vertices(g3));
   std::vector<std::pair<int, int> > edge_pairs_g3;
   IndexMap2 index_map3 = boost::get(boost::vertex_index_t(), g3);
   BGL_FORALL_EDGES_T(e, g3, Graph2) {
     edge_pairs_g3.push_back(
       std::make_pair(get(index_map3, source(e, g3)),
                      get(index_map3, target(e, g3))));
   }
   // Normalize the edge pairs for comparison
   if (boost::is_convertible<typename boost::graph_traits<Graph1>::directed_category*, boost::undirected_tag*>::value || boost::is_convertible<typename boost::graph_traits<Graph2>::directed_category*, boost::undirected_tag*>::value) {
     for (size_t i = 0; i < edge_pairs_g1.size(); ++i) {
       if (edge_pairs_g1[i].first < edge_pairs_g1[i].second) {
         std::swap(edge_pairs_g1[i].first, edge_pairs_g1[i].second);
       }
     }
     for (size_t i = 0; i < edge_pairs_g3.size(); ++i) {
       if (edge_pairs_g3[i].first < edge_pairs_g3[i].second) {
         std::swap(edge_pairs_g3[i].first, edge_pairs_g3[i].second);
       }
     }
   }
   std::sort(edge_pairs_g1.begin(), edge_pairs_g1.end());
   std::sort(edge_pairs_g3.begin(), edge_pairs_g3.end());
   edge_pairs_g1.erase(std::unique(edge_pairs_g1.begin(), edge_pairs_g1.end()), edge_pairs_g1.end());
   edge_pairs_g3.erase(std::unique(edge_pairs_g3.begin(), edge_pairs_g3.end()), edge_pairs_g3.end());
   BOOST_CHECK(edge_pairs_g1 == edge_pairs_g3);
}

template <typename Graph>
void test_remove_edges()
{
    using namespace boost;

    // Build a 2-vertex graph
    Graph g(2);
    add_edge(vertex(0, g), vertex(1, g), g);
    BOOST_CHECK(num_vertices(g) == 2);
    BOOST_CHECK(num_edges(g) == 1);
    remove_edge(vertex(0, g), vertex(1, g), g);
    BOOST_CHECK(num_edges(g) == 0);

    // Make sure we don't decrement the edge count if the edge doesn't actually
    // exist.
    remove_edge(vertex(0, g), vertex(1, g), g);
    BOOST_CHECK(num_edges(g) == 0);
}


int test_main(int, char*[])
{
        // Use setS to keep out edges in order, so they match the adjacency_matrix.
    typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS>
            UGraph1;
    typedef boost::adjacency_matrix<boost::undirectedS>
            UGraph2;
    run_test<UGraph1, UGraph2>();

        // Use setS to keep out edges in order, so they match the adjacency_matrix.
    typedef boost::adjacency_list<boost::setS, boost::vecS,
                                    boost::bidirectionalS>
            BGraph1;
    typedef boost::adjacency_matrix<boost::directedS>
            BGraph2;
    run_test<BGraph1, BGraph2>();

    test_remove_edges<UGraph2>();
    test_remove_edges<BGraph2>();

    return 0;
}

