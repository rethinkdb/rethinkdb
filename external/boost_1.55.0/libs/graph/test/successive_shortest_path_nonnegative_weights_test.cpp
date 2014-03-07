//=======================================================================
// Copyright 2013 University of Warsaw.
// Authors: Piotr Wygocki 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#define BOOST_TEST_MODULE successive_shortest_path_nonnegative_weights_test

#include <boost/test/unit_test.hpp>

#include <boost/graph/successive_shortest_path_nonnegative_weights.hpp>
#include <boost/graph/find_flow_cost.hpp>

#include "min_cost_max_flow_utils.hpp"


BOOST_AUTO_TEST_CASE(path_augmentation_def_test) {
    boost::SampleGraph::vertex_descriptor s,t;
    boost::SampleGraph::Graph g; 
    boost::SampleGraph::getSampleGraph(g, s, t);

    boost::successive_shortest_path_nonnegative_weights(g, s, t);

    int cost =  boost::find_flow_cost(g);
    BOOST_CHECK_EQUAL(cost, 29);
}

BOOST_AUTO_TEST_CASE(path_augmentation_def_test2) {
    boost::SampleGraph::vertex_descriptor s,t;
    boost::SampleGraph::Graph g; 
    boost::SampleGraph::getSampleGraph2(g, s, t);

    boost::successive_shortest_path_nonnegative_weights(g, s, t);

    int cost =  boost::find_flow_cost(g);
    BOOST_CHECK_EQUAL(cost, 7);
}

BOOST_AUTO_TEST_CASE(path_augmentation_test) {
    boost::SampleGraph::vertex_descriptor s,t;
    typedef boost::SampleGraph::Graph Graph;
    Graph g;
    boost::SampleGraph::getSampleGraph(g, s, t);

    int N = boost::num_vertices(g);
    std::vector<int> dist(N);
    std::vector<int> dist_prev(N);
    typedef boost::graph_traits<Graph>::edge_descriptor edge_descriptor;
    std::vector<edge_descriptor> pred(N);
        
    boost::property_map<Graph, boost::vertex_index_t>::const_type
      idx = get(boost::vertex_index, g);

    boost::successive_shortest_path_nonnegative_weights(g, s, t, 
            boost::distance_map(boost::make_iterator_property_map(dist.begin(), idx)).
            predecessor_map(boost::make_iterator_property_map(pred.begin(), idx)).
            distance_map2(boost::make_iterator_property_map(dist_prev.begin(), idx)).
            vertex_index_map(idx));

    int cost =  boost::find_flow_cost(g);
    BOOST_CHECK_EQUAL(cost, 29);
}


