// Copyright (C) 2006-2009 Dmitry Bufistov and Andrey Parfenov

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <ctime>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/howard_cycle_ratio.hpp>

/**
 * @author Dmitry Bufistov
 * @author Andrey Parfenov
 */

using namespace boost;
typedef adjacency_list<
    listS, listS, directedS,
    property<vertex_index_t, int>,
    property<
        edge_weight_t, double, property<edge_weight2_t, double>
    >
> grap_real_t;

template <typename TG>
void gen_rand_graph(TG &g, size_t nV, size_t nE)
{
    g.clear();
    mt19937 rng;
    rng.seed(uint32_t(time(0)));
    boost::generate_random_graph(g, nV, nE, rng, true, true);
    boost::uniform_real<> ur(-1,10);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<> >   ew1rg(rng, ur);
    randomize_property<edge_weight_t>(g, ew1rg);
    boost::uniform_int<size_t> uint(1,5);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<size_t> >      ew2rg(rng, uint);
    randomize_property<edge_weight2_t>(g, ew2rg);
}

int main(int argc, char* argv[])
{
    using std::cout;
    using std::endl;
    const double epsilon = 0.0000001;
    double min_cr, max_cr; ///Minimum and maximum cycle ratio
    typedef std::vector<graph_traits<grap_real_t>::edge_descriptor> ccReal_t;
    ccReal_t cc; ///critical cycle

    grap_real_t tgr;
    property_map<grap_real_t, vertex_index_t>::type vim = get(vertex_index, tgr);
    property_map<grap_real_t, edge_weight_t>::type ew1 = get(edge_weight, tgr);
    property_map<grap_real_t, edge_weight2_t>::type ew2 = get(edge_weight2, tgr);

    gen_rand_graph(tgr, 1000, 30000);
    cout << "Vertices number: " << num_vertices(tgr) << endl;
    cout << "Edges number: " << num_edges(tgr) << endl;
    int i = 0;
    graph_traits<grap_real_t>::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(tgr); vi != vi_end; vi++) {
        vim[*vi] = i++; ///Initialize vertex index property
    }
    max_cr = maximum_cycle_ratio(tgr, vim, ew1, ew2);
    cout << "Maximum cycle ratio is " << max_cr << endl;
    min_cr = minimum_cycle_ratio(tgr, vim, ew1, ew2, &cc);
    cout << "Minimum cycle ratio is " << min_cr << endl;
    std::pair<double, double> cr(.0,.0);
    cout << "Critical cycle:\n";
    for (ccReal_t::iterator itr = cc.begin(); itr != cc.end(); ++itr)
    {
        cr.first += ew1[*itr];
        cr.second += ew2[*itr];
        std::cout << "(" << vim[source(*itr, tgr)] << "," <<
            vim[target(*itr, tgr)] << ") ";
    }
    cout << endl;
    assert(std::abs(cr.first / cr.second - min_cr) < epsilon);
    return EXIT_SUCCESS;
}

