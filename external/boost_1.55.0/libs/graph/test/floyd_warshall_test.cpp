// Copyright 2002 Rensselaer Polytechnic Institute

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Lauren Foutz
//           Scott Hill
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <map>
#include <algorithm>
#include <iostream>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/bellman_ford_shortest_paths.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/test/minimal.hpp>
#include <algorithm>
using namespace boost;

template<typename T>
inline const T& my_min(const T& x, const T& y) 
{ return x < y? x : y; }

template<typename Graph>
bool acceptance_test(Graph& g, int vec, int e)
{
  boost::minstd_rand ran(vec);

  {
    typename boost::property_map<Graph, boost::vertex_name_t>::type index =
      boost::get(boost::vertex_name, g);
    typename boost::graph_traits<Graph>::vertex_iterator firstv, lastv,
      firstv2, lastv2;
    int x = 0;
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      boost::put(index, *firstv, x);
      x++;
    }


    for(int i = 0; i < e; i++){
      boost::add_edge(index[ran() % vec], index[ran() % vec], g);
    }


    typename boost::graph_traits<Graph>::edge_iterator first, last;
    typename boost::property_map<Graph, boost::edge_weight_t>::type
      local_edge_map = boost::get(boost::edge_weight, g);
    for(boost::tie(first, last) = boost::edges(g); first != last; first++){
      if (ran() % vec != 0){
        boost::put(local_edge_map, *first, ran() % 100);
      } else {
        boost::put(local_edge_map, *first, 0 - (ran() % 100));
      }
    }

    int int_inf =
      std::numeric_limits<int>::max BOOST_PREVENT_MACRO_SUBSTITUTION();
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_des;
    std::map<vertex_des,int> matrixRow;
    std::map<vertex_des, std::map<vertex_des ,int> > matrix;
    typedef typename boost::property_map<Graph, boost::vertex_distance_t>::type
      distance_type;
    distance_type distance_row = boost::get(boost::vertex_distance, g);
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      boost::put(distance_row, *firstv, int_inf);
      matrixRow[*firstv] = int_inf;
    }
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      matrix[*firstv] = matrixRow;
    }
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      matrix[*firstv][*firstv] = 0;
    }
    std::map<vertex_des, std::map<vertex_des, int> > matrix3(matrix);
    std::map<vertex_des, std::map<vertex_des, int> > matrix4(matrix);
    for(boost::tie(first, last) = boost::edges(g); first != last; first++){
      if (matrix[boost::source(*first, g)][boost::target(*first, g)] != int_inf)
      {
        matrix[boost::source(*first, g)][boost::target(*first, g)] =
          my_min
            (boost::get(local_edge_map, *first),
             matrix[boost::source(*first, g)][boost::target(*first, g)]);
      } else {
        matrix[boost::source(*first, g)][boost::target(*first, g)] =
          boost::get(local_edge_map, *first);
      }
    }
    bool is_undirected =
      boost::is_same<typename boost::graph_traits<Graph>::directed_category,
      boost::undirected_tag>::value;
    if (is_undirected){
      for(boost::tie(first, last) = boost::edges(g); first != last; first++){
        if (matrix[boost::target(*first, g)][boost::source(*first, g)] != int_inf)
        {
          matrix[boost::target(*first, g)][boost::source(*first, g)] =
            my_min
              (boost::get(local_edge_map, *first),
               matrix[boost::target(*first, g)][boost::source(*first, g)]);
        } else {
          matrix[boost::target(*first, g)][boost::source(*first, g)] =
            boost::get(local_edge_map, *first);
        }
      }
    }


    bool bellman, floyd1, floyd2, floyd3;
    floyd1 =
      boost::floyd_warshall_initialized_all_pairs_shortest_paths
        (g,
         matrix, weight_map(boost::get(boost::edge_weight, g)).
         distance_inf(int_inf). distance_zero(0));

    floyd2 =
      boost::floyd_warshall_all_pairs_shortest_paths
        (g, matrix3,
         weight_map(local_edge_map).
         distance_inf(int_inf). distance_zero(0));

    floyd3 = boost::floyd_warshall_all_pairs_shortest_paths(g, matrix4);


    boost::dummy_property_map dummy_map;
    std::map<vertex_des, std::map<vertex_des, int> > matrix2;
    for(boost::tie(firstv, lastv) = vertices(g); firstv != lastv; firstv++){
      boost::put(distance_row, *firstv, 0);
      bellman =
        boost::bellman_ford_shortest_paths
          (g, vec,
           weight_map(boost::get(boost::edge_weight, g)).
           distance_map(boost::get(boost::vertex_distance, g)).
           predecessor_map(dummy_map));
      distance_row = boost::get(boost::vertex_distance, g);
      for(boost::tie(firstv2, lastv2) = vertices(g); firstv2 != lastv2;
          firstv2++){
        matrix2[*firstv][*firstv2] = boost::get(distance_row, *firstv2);
        boost::put(distance_row, *firstv2, int_inf);
      }
      if(bellman == false){
        break;
      }
    }


    if (bellman != floyd1 || bellman != floyd2 || bellman != floyd3){
      std::cout <<
        "A negative cycle was detected in one algorithm but not the others. "
                << std::endl;
      return false;
    }
    else if (bellman == false && floyd1 == false && floyd2 == false &&
             floyd3 == false){
      return true;
    }
    else {
      typename boost::graph_traits<Graph>::vertex_iterator first1, first2,
        last1, last2;
      for (boost::tie(first1, last1) = boost::vertices(g); first1 != last1;
           first1++){
        for (boost::tie(first2, last2) = boost::vertices(g); first2 != last2;
             first2++){
          if (matrix2[*first1][*first2] != matrix[*first1][*first2]){
            std::cout << "Algorithms do not match at matrix point "
                      << index[*first1] << " " << index[*first2]
                      << " Bellman results: " << matrix2[*first1][*first2]
                      << " floyd 1 results " << matrix[*first1][*first2]
                      << std::endl;
            return false;
          }
          if (matrix2[*first1][*first2] != matrix3[*first1][*first2]){
            std::cout << "Algorithms do not match at matrix point "
                      << index[*first1] << " " << index[*first2]
                      << " Bellman results: " << matrix2[*first1][*first2]
                      << " floyd 2 results " << matrix3[*first1][*first2]
                      << std::endl;
            return false;
          }
          if (matrix2[*first1][*first2] != matrix4[*first1][*first2]){
            std::cout << "Algorithms do not match at matrix point "
                      << index[*first1] << " " << index[*first2]
                      << " Bellman results: " << matrix2[*first1][*first2]
                      << " floyd 3 results " << matrix4[*first1][*first2]
                      << std::endl;
            return false;
          }
        }
      }
    }

  }
  return true;
}

template<typename Graph>
bool acceptance_test2(Graph& g, int vec, int e)
{
  boost::minstd_rand ran(vec);

  {

    typename boost::property_map<Graph, boost::vertex_name_t>::type index =
      boost::get(boost::vertex_name, g);
    typename boost::graph_traits<Graph>::vertex_iterator firstv, lastv,
      firstv2, lastv2;
    int x = 0;
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      boost::put(index, *firstv, x);
      x++;
    }

    boost::generate_random_graph(g, vec, e, ran, true);

    typename boost::graph_traits<Graph>::edge_iterator first, last;
    typename boost::property_map<Graph, boost::edge_weight_t>::type
      local_edge_map = boost::get(boost::edge_weight, g);
    for(boost::tie(first, last) = boost::edges(g); first != last; first++){
      if (ran() % vec != 0){
        boost::put(local_edge_map, *first, ran() % 100);
      } else {
        boost::put(local_edge_map, *first, 0 - (ran() % 100));
      }
    }

    int int_inf =
      std::numeric_limits<int>::max BOOST_PREVENT_MACRO_SUBSTITUTION();
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_des;
    std::map<vertex_des,int> matrixRow;
    std::map<vertex_des, std::map<vertex_des ,int> > matrix;
    typedef typename boost::property_map<Graph, boost::vertex_distance_t>::type
      distance_type;
    distance_type distance_row = boost::get(boost::vertex_distance, g);
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      boost::put(distance_row, *firstv, int_inf);
      matrixRow[*firstv] = int_inf;
    }
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      matrix[*firstv] = matrixRow;
    }
    for(boost::tie(firstv, lastv) = boost::vertices(g); firstv != lastv;
        firstv++){
      matrix[*firstv][*firstv] = 0;
    }
    std::map<vertex_des, std::map<vertex_des, int> > matrix3(matrix);
    std::map<vertex_des, std::map<vertex_des, int> > matrix4(matrix);
    for(boost::tie(first, last) = boost::edges(g); first != last; first++){
      if (matrix[boost::source(*first, g)][boost::target(*first, g)] != int_inf)
      {
        matrix[boost::source(*first, g)][boost::target(*first, g)] =
          my_min
            (boost::get(local_edge_map, *first),
             matrix[boost::source(*first, g)][boost::target(*first, g)]);
      } else {
        matrix[boost::source(*first, g)][boost::target(*first, g)] =
          boost::get(local_edge_map, *first);
      }
    }
    bool is_undirected =
      boost::is_same<typename boost::graph_traits<Graph>::directed_category,
      boost::undirected_tag>::value;
    if (is_undirected){
      for(boost::tie(first, last) = boost::edges(g); first != last; first++){
        if (matrix[boost::target(*first, g)][boost::source(*first, g)]
            != int_inf){
          matrix[boost::target(*first, g)][boost::source(*first, g)] =
            my_min
              (boost::get(local_edge_map, *first),
               matrix[boost::target(*first, g)][boost::source(*first, g)]);
        } else {
          matrix[boost::target(*first, g)][boost::source(*first, g)] =
            boost::get(local_edge_map, *first);
        }
      }
    }


    bool bellman, floyd1, floyd2, floyd3;
    floyd1 =
      boost::floyd_warshall_initialized_all_pairs_shortest_paths
        (g,
         matrix, weight_map(boost::get(boost::edge_weight, g)).
         distance_inf(int_inf). distance_zero(0));

    floyd2 =
      boost::floyd_warshall_all_pairs_shortest_paths
        (g, matrix3,
         weight_map(local_edge_map).
         distance_inf(int_inf). distance_zero(0));

    floyd3 = boost::floyd_warshall_all_pairs_shortest_paths(g, matrix4);


    boost::dummy_property_map dummy_map;
    std::map<vertex_des, std::map<vertex_des, int> > matrix2;
    for(boost::tie(firstv, lastv) = vertices(g); firstv != lastv; firstv++){
      boost::put(distance_row, *firstv, 0);
      bellman =
        boost::bellman_ford_shortest_paths
          (g, vec,
           weight_map(boost::get(boost::edge_weight, g)).
           distance_map(boost::get(boost::vertex_distance, g)).
           predecessor_map(dummy_map));
      distance_row = boost::get(boost::vertex_distance, g);
      for(boost::tie(firstv2, lastv2) = vertices(g); firstv2 != lastv2;
          firstv2++){
        matrix2[*firstv][*firstv2] = boost::get(distance_row, *firstv2);
        boost::put(distance_row, *firstv2, int_inf);
      }
      if(bellman == false){
        break;
      }
    }


    if (bellman != floyd1 || bellman != floyd2 || bellman != floyd3){
      std::cout <<
        "A negative cycle was detected in one algorithm but not the others. "
                << std::endl;
      return false;
    }
    else if (bellman == false && floyd1 == false && floyd2 == false &&
             floyd3 == false){
      return true;
    }
    else {
      typename boost::graph_traits<Graph>::vertex_iterator first1, first2,
        last1, last2;
      for (boost::tie(first1, last1) = boost::vertices(g); first1 != last1;
           first1++){
        for (boost::tie(first2, last2) = boost::vertices(g); first2 != last2;
             first2++){
          if (matrix2[*first1][*first2] != matrix[*first1][*first2]){
            std::cout << "Algorithms do not match at matrix point "
                      << index[*first1] << " " << index[*first2]
                      << " Bellman results: " << matrix2[*first1][*first2]
                      << " floyd 1 results " << matrix[*first1][*first2]
                      << std::endl;
            return false;
          }
          if (matrix2[*first1][*first2] != matrix3[*first1][*first2]){
            std::cout << "Algorithms do not match at matrix point "
                      << index[*first1] << " " << index[*first2]
                      << " Bellman results: " << matrix2[*first1][*first2]
                      << " floyd 2 results " << matrix3[*first1][*first2]
                      << std::endl;
            return false;
          }
          if (matrix2[*first1][*first2] != matrix4[*first1][*first2]){
            std::cout << "Algorithms do not match at matrix point "
                      << index[*first1] << " " << index[*first2]
                      << " Bellman results: " << matrix2[*first1][*first2]
                      << " floyd 3 results " << matrix4[*first1][*first2]
                      << std::endl;
            return false;
          }
        }
      }
    }

  }
  return true;
}

int test_main(int, char*[])
{
  typedef boost::adjacency_list<boost::listS, boost::listS, boost::directedS,
            boost::property<boost::vertex_distance_t, int,
            boost::property<boost::vertex_name_t, int> > ,
            boost::property<boost::edge_weight_t, int> > Digraph;
  Digraph adjlist_digraph;
  BOOST_CHECK(acceptance_test2(adjlist_digraph, 100, 2000));

  typedef boost::adjacency_matrix<boost::undirectedS,
            boost::property<boost::vertex_distance_t, int,
            boost::property<boost::vertex_name_t, int> > ,
            boost::property<boost::edge_weight_t, int> > Graph;
  Graph matrix_graph(100);
  BOOST_CHECK(acceptance_test(matrix_graph, 100, 2000));

  return 0;
}
