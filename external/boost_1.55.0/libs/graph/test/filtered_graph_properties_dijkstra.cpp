// (c) Copyright Juergen Hunold 2012
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/filtered_graph.hpp>

namespace boost {

    enum edge_info_t { edge_info = 114 };

    BOOST_INSTALL_PROPERTY( edge, info );
}

template< typename EdgeInfo,
          typename Directed >
class Graph
{
public:
    typedef boost::property< boost::edge_info_t, EdgeInfo > tEdge_property;

    typedef boost::adjacency_list< boost::setS,
                                   boost::vecS,
                                   Directed,
                                   boost::no_property,
                                   tEdge_property > tGraph;

    typedef typename boost::graph_traits< tGraph >::vertex_descriptor tNode;
    typedef typename boost::graph_traits< tGraph >::edge_descriptor   tEdge;

protected:

    tGraph          m_Graph;
};

class DataEdge;

class UndirectedGraph
    : public Graph< DataEdge*,
                    boost::undirectedS >
{
public:

    template< class Evaluator, class Filter >
    void        dijkstra( Evaluator const&,
                          Filter const& ) const;
};

template< typename Graph, typename Derived >
struct Evaluator : public boost::put_get_helper< int, Derived >
{
    typedef int value_type;
    typedef typename Graph::tEdge key_type;
    typedef int reference;
    typedef boost::readable_property_map_tag category;

    explicit Evaluator( Graph const* pGraph );
};


template< typename Graph >
struct LengthEvaluator : public Evaluator< Graph, LengthEvaluator<Graph> >
{
    explicit LengthEvaluator( Graph const* pGraph );

    typedef typename Evaluator<Graph, LengthEvaluator<Graph> >::reference reference;
    typedef typename Evaluator<Graph, LengthEvaluator<Graph> >::key_type key_type;

    virtual reference operator[] ( key_type const& edge ) const;
};

template< class Graph >
struct EdgeFilter
{
    typedef typename Graph::tEdge key_type;

    EdgeFilter();

    explicit EdgeFilter( Graph const*);

    bool    operator()( key_type const& ) const;

private:
    const Graph*    m_pGraph;
};


template< class Evaluator, class Filter >
void
UndirectedGraph::dijkstra( Evaluator const& rEvaluator,
                           Filter const& rFilter ) const
{
    tNode nodeSource = vertex(0, m_Graph);

    std::vector< tNode > predecessors( num_vertices(m_Graph) );

    boost::filtered_graph< tGraph, Filter > filteredGraph( m_Graph, rFilter );

    boost::dijkstra_shortest_paths( filteredGraph,
                                    nodeSource,
                                    boost::predecessor_map( &predecessors[0] )
                                    .weight_map( rEvaluator ) );
}

// explicit instantiation
template void UndirectedGraph::dijkstra( LengthEvaluator<UndirectedGraph> const&,
                                         EdgeFilter<UndirectedGraph> const& ) const;

int main(int, char**) {return 0;} // Tests above will fail to compile if anything is broken
