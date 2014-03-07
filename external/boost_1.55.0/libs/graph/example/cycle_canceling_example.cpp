//=======================================================================
// Copyright 2013 University of Warsaw.
// Authors: Piotr Wygocki 
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <boost/graph/cycle_canceling.hpp>
#include <boost/graph/edmonds_karp_max_flow.hpp>

#include "../test/min_cost_max_flow_utils.hpp"


int main() {
    boost::SampleGraph::vertex_descriptor s,t;
    boost::SampleGraph::Graph g;
    boost::SampleGraph::getSampleGraph(g, s, t);

    boost::edmonds_karp_max_flow(g, s, t);
    boost::cycle_canceling(g);

    int cost = boost::find_flow_cost(g);
    assert(cost == 29);
    return 0;
}

