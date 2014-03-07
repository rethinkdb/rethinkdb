// Copyright Louis Dionne 2013

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/assert.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/hawick_circuits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/next_prior.hpp>
#include <boost/property_map/property_map.hpp>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <map>


template <typename OutputStream>
struct cycle_printer
{
    cycle_printer(OutputStream& stream)
        : os(stream)
    { }

    template <typename Path, typename Graph>
    void cycle(Path const& p, Graph const& g)
    {
        if (p.empty())
            return;

        // Get the property map containing the vertex indices
        // so we can print them.
        typedef typename boost::property_map<
                    Graph, boost::vertex_index_t
                >::const_type IndexMap;

        IndexMap indices = get(boost::vertex_index, g);

        // Iterate over path printing each vertex that forms the cycle.
        typename Path::const_iterator i, before_end = boost::prior(p.end());
        for (i = p.begin(); i != before_end; ++i) {
            os << get(indices, *i) << " ";
        }
        os << get(indices, *i) << '\n';
    }
    OutputStream& os;
};


// VertexPairIterator is an iterator over pairs of whitespace separated
// vertices `u` and `v` representing a directed edge from `u` to `v`.
template <typename Graph, typename VertexPairIterator>
void build_graph(Graph& graph, unsigned int const nvertices,
                 VertexPairIterator first, VertexPairIterator last) {
    typedef boost::graph_traits<Graph> Traits;
    typedef typename Traits::vertex_descriptor vertex_descriptor;
    std::map<unsigned int, vertex_descriptor> vertices;

    for (unsigned int i = 0; i < nvertices; ++i)
        vertices[i] = add_vertex(graph);

    for (; first != last; ++first) {
        unsigned int u = *first++;

        BOOST_ASSERT_MSG(first != last,
            "there is a lonely vertex at the end of the edge list");

        unsigned int v = *first;

        BOOST_ASSERT_MSG(vertices.count(u) == 1 && vertices.count(v) == 1,
            "specified a vertex over the number of vertices in the graph");

        add_edge(vertices[u], vertices[v], graph);
    }
    BOOST_ASSERT(num_vertices(graph) == nvertices);
}


int main(int argc, char const* argv[]) {
    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " num_vertices < input\n";
        return EXIT_FAILURE;
    }

    unsigned int num_vertices = boost::lexical_cast<unsigned int>(argv[1]);
    std::istream_iterator<unsigned int> first_vertex(std::cin), last_vertex;
    boost::directed_graph<> graph;
    build_graph(graph, num_vertices, first_vertex, last_vertex);

    cycle_printer<std::ostream> visitor(std::cout);
    boost::hawick_circuits(graph, visitor);

    return EXIT_SUCCESS;
}
