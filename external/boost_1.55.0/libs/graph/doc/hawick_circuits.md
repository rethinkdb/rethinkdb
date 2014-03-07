<body bgcolor="#ffffff" link="#0000ee" text="#000000" vlink="#551a8b" alink="#ff0000">

![C++ Boost](../../../boost.png)

# `hawick_circuits`

    template <typename Graph, typename Visitor, typename VertexIndexMap>
    void hawick_circuits(Graph const& graph, Visitor visitor, VertexIndexMap const& vim = get(vertex_index, graph));

    template <typename Graph, typename Visitor, typename VertexIndexMap>
    void hawick_unique_circuits(Graph const& graph, Visitor visitor, VertexIndexMap const& vim = get(vertex_index, graph));

Enumerate all the elementary circuits in a directed multigraph. Specifically,
self-loops and redundant circuits caused by parallel edges are enumerated too.
`hawick_unique_circuits` may be used if redundant circuits caused by parallel
edges are not desired.

The algorithm is described in detail in
<http://www.massey.ac.nz/~kahawick/cstn/013/cstn-013.pdf>.


### Where defined

[`#include <boost/graph/hawick_circuits.hpp>`](../../../boost/graph/hawick_circuits.hpp)


### Parameters

__IN:__ `Graph const& graph`

> The graph on which the algorithm is to be performed. It must be a model of
> the `VertexListGraph` and `AdjacencyGraph` concepts.

__IN:__ `Visitor visitor`

> The visitor that will be notified on each circuit found by the algorithm.
> The `visitor.cycle(circuit, graph)` expression must be valid, with `circuit`
> being a `const`-reference to a random access sequence of `vertex_descriptor`s.
>
> For example, if a circuit `u -> v -> w -> u` exists in the graph, the
> visitor will be called with a sequence consisting of `(u, v, w)`.

__IN:__ `VertexIndexMap const& vim = get(vertex_index, graph)`

> A model of the `ReadablePropertyMap` concept mapping each `vertex_descriptor`
> to an integer in the range `[0, num_vertices(graph))`. It defaults to using
> the vertex index map provided by the `graph`.


------------------------------------------------------------------------------
<div class="footer">
    &copy; 2013 Louis Dionne
</div>
