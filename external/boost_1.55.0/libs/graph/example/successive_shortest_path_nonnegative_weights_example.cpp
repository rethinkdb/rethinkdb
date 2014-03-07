//=======================================================================
// Copyright 2013 University of Warsaw.
// Authors: Piotr Wygocki 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/successive_shortest_path_nonnegative_weights.hpp>
#include <boost/graph/find_flow_cost.hpp>

#include "../test/min_cost_max_flow_utils.hpp"


int main() {
    boost::SampleGraph::vertex_descriptor s,t;
    boost::SampleGraph::Graph g; 
    boost::SampleGraph::getSampleGraph(g, s, t);

    boost::successive_shortest_path_nonnegative_weights(g, s, t);

    int cost =  boost::find_flow_cost(g);
    assert(cost == 29);

    return 0;
}



