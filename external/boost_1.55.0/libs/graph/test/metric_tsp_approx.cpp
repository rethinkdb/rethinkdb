//=======================================================================
// Copyright 2008
// Author: Matyas W Egyhazy
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <iostream>
#include <vector>
#include <fstream>
#include <set>
#include <ctime>

#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>
#include <boost/timer.hpp>
#include <boost/integer_traits.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/simple_point.hpp>
#include <boost/graph/metric_tsp_approx.hpp>
#include <boost/graph/graphviz.hpp>

// TODO: Integrate this into the test system a little better. We need to run
// the test with some kind of input file.

template<typename PointType>
struct cmpPnt
{
    bool operator()(const boost::simple_point<PointType>& l,
                    const boost::simple_point<PointType>& r) const
    { return (l.x > r.x); }
};

//add edges to the graph (for each node connect it to all other nodes)
template<typename VertexListGraph, typename PointContainer,
    typename WeightMap, typename VertexIndexMap>
void connectAllEuclidean(VertexListGraph& g,
                        const PointContainer& points,
                        WeightMap wmap,            // Property maps passed by value
                        VertexIndexMap vmap,       // Property maps passed by value
                        int /*sz*/)
{
    using namespace boost;
    using namespace std;
    typedef typename graph_traits<VertexListGraph>::edge_descriptor Edge;
    typedef typename graph_traits<VertexListGraph>::vertex_iterator VItr;

    Edge e;
    bool inserted;

    pair<VItr, VItr> verts(vertices(g));
    for (VItr src(verts.first); src != verts.second; src++)
    {
        for (VItr dest(src); dest != verts.second; dest++)
        {
            if (dest != src)
            {
                double weight(sqrt(pow(
                    static_cast<double>(points[vmap[*src]].x -
                        points[vmap[*dest]].x), 2.0) +
                    pow(static_cast<double>(points[vmap[*dest]].y -
                        points[vmap[*src]].y), 2.0)));

                boost::tie(e, inserted) = add_edge(*src, *dest, g);

                wmap[e] = weight;
            }

        }

    }
}

// Create a randomly generated point
// scatter time execution
void testScalability(unsigned numpts)
{
    using namespace boost;
    using namespace std;

    typedef adjacency_matrix<undirectedS, no_property,
        property <edge_weight_t, double,
        property<edge_index_t, int> > > Graph;
    typedef graph_traits<Graph>::vertex_descriptor Vertex;
    typedef property_map<Graph, edge_weight_t>::type WeightMap;
    typedef set<simple_point<double>, cmpPnt<double> > PointSet;
    typedef vector< Vertex > Container;

    boost::mt19937 rng(time(0));
    uniform_real<> range(0.01, (numpts * 2));
    variate_generator<boost::mt19937&, uniform_real<> >
        pnt_gen(rng, range);

    PointSet points;
    simple_point<double> pnt;

    while (points.size() < numpts)
    {
        pnt.x = pnt_gen();
        pnt.y = pnt_gen();
        points.insert(pnt);
    }

    Graph g(numpts);
    WeightMap weight_map(get(edge_weight, g));
    vector<simple_point<double> > point_vec(points.begin(), points.end());

    connectAllEuclidean(g, point_vec, weight_map, get(vertex_index, g), numpts);

    Container c;
    timer t;
    double len = 0.0;

    // Run the TSP approx, creating the visitor on the fly.
    metric_tsp_approx(g, make_tsp_tour_len_visitor(g, back_inserter(c), len, weight_map));

    cout << "Number of points: " << num_vertices(g) << endl;
    cout << "Number of edges: " << num_edges(g) << endl;
    cout << "Length of tour: " << len << endl;
    cout << "Elapsed: " << t.elapsed() << endl;
}

template <typename PositionVec>
void checkAdjList(PositionVec v)
{
    using namespace std;
    using namespace boost;

    typedef adjacency_list<listS, listS, undirectedS> Graph;
    typedef graph_traits<Graph>::vertex_descriptor Vertex;
    typedef graph_traits <Graph>::edge_descriptor Edge;
    typedef vector<Vertex> Container;
    typedef map<Vertex, std::size_t> VertexIndexMap;
    typedef map<Edge, double> EdgeWeightMap;
    typedef associative_property_map<VertexIndexMap> VPropertyMap;
    typedef associative_property_map<EdgeWeightMap> EWeightPropertyMap;
    typedef graph_traits<Graph>::vertex_iterator VItr;

    Container c;
    EdgeWeightMap w_map;
    VertexIndexMap v_map;
    VPropertyMap v_pmap(v_map);
    EWeightPropertyMap w_pmap(w_map);

    Graph g(v.size());

    //create vertex index map
    VItr vi, ve;
    int idx(0);
    for (boost::tie(vi, ve) = vertices(g); vi != ve; ++vi)
    {
        Vertex v(*vi);
        v_pmap[v] = idx;
        idx++;
    }

    connectAllEuclidean(g, v, w_pmap,
        v_pmap, v.size());

    metric_tsp_approx_from_vertex(g,
        *vertices(g).first,
        w_pmap,
        v_pmap,
        tsp_tour_visitor<back_insert_iterator<Container > >
        (back_inserter(c)));

    cout << "adj_list" << endl;
    for (Container::iterator itr = c.begin(); itr != c.end(); ++itr) {
        cout << v_map[*itr] << " ";
    }
    cout << endl << endl;

    c.clear();
}

static void usage()
{
    using namespace std;
    cerr << "To run this program properly please place a "
         << "file called graph.txt"
         << endl << "into the current working directory." << endl
         << "Each line of this file should be a coordinate specifying the"
         << endl << "location of a vertex" << endl
         << "For example: " << endl << "1,2" << endl << "20,4" << endl
         << "15,7" << endl << endl;
}

int main(int argc, char* argv[])
{
   using namespace boost;
   using namespace std;

    typedef vector<simple_point<double> > PositionVec;
    typedef adjacency_matrix<undirectedS, no_property,
        property <edge_weight_t, double> > Graph;
    typedef graph_traits<Graph>::vertex_descriptor Vertex;
    typedef vector<Vertex> Container;
    typedef property_map<Graph, edge_weight_t>::type WeightMap;
    typedef property_map<Graph, vertex_index_t>::type VertexMap;

    // Make sure that the the we can parse the given file.
    if(argc < 2) {
        usage();
        // return -1;
        return 0;
    }

    // Open the graph file, failing if one isn't given on the command line.
    ifstream fin(argv[1]);
    if (!fin)
    {
        usage();
        // return -1;
        return 0;
    }

   string line;
   PositionVec position_vec;

   int n(0);
   while (getline(fin, line))
   {
       simple_point<double> vertex;

       size_t idx(line.find(","));
       string xStr(line.substr(0, idx));
       string yStr(line.substr(idx + 1, line.size() - idx));

       vertex.x = lexical_cast<double>(xStr);
       vertex.y = lexical_cast<double>(yStr);

       position_vec.push_back(vertex);
       n++;
   }

   fin.close();

   Container c;
   Graph g(position_vec.size());
   WeightMap weight_map(get(edge_weight, g));
   VertexMap v_map = get(vertex_index, g);

   connectAllEuclidean(g, position_vec, weight_map, v_map, n);

   metric_tsp_approx_tour(g, back_inserter(c));

   for (vector<Vertex>::iterator itr = c.begin(); itr != c.end(); ++itr)
   {
       cout << *itr << " ";
   }
   cout << endl << endl;

   c.clear();

   checkAdjList(position_vec);

   metric_tsp_approx_from_vertex(g, *vertices(g).first,
       get(edge_weight, g), get(vertex_index, g),
       tsp_tour_visitor<back_insert_iterator<vector<Vertex> > >
       (back_inserter(c)));

   for (vector<Vertex>::iterator itr = c.begin(); itr != c.end(); ++itr)
   {
       cout << *itr << " ";
   }
   cout << endl << endl;

   c.clear();

   double len(0.0);
   try {
       metric_tsp_approx(g, make_tsp_tour_len_visitor(g, back_inserter(c), len, weight_map));
   }
   catch (const bad_graph& e) {
       cerr << "bad_graph: " << e.what() << endl;
       return -1;
   }

   cout << "Number of points: " << num_vertices(g) << endl;
   cout << "Number of edges: " << num_edges(g) << endl;
   cout << "Length of Tour: " << len << endl;

   int cnt(0);
   pair<Vertex,Vertex> triangleEdge;
   for (vector<Vertex>::iterator itr = c.begin(); itr != c.end();
       ++itr, ++cnt)
   {
       cout << *itr << " ";

       if (cnt == 2)
       {
           triangleEdge.first = *itr;
       }
       if (cnt == 3)
       {
           triangleEdge.second = *itr;
       }
   }
   cout << endl << endl;
   c.clear();

   testScalability(1000);

   // if the graph is not fully connected then some of the
   // assumed triangle-inequality edges may not exist
   remove_edge(edge(triangleEdge.first, triangleEdge.second, g).first, g);

    // Make sure that we can actually trap incomplete graphs.
    bool caught = false;
    try {
        double len = 0.0;
        metric_tsp_approx(g, make_tsp_tour_len_visitor(g, back_inserter(c), len, weight_map));
    }
    catch (const bad_graph& e) { caught = true; }
    BOOST_ASSERT(caught);

   return 0;
}
