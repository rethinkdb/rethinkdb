.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

=================================
|Logo| Distributed Adjacency List
=================================

.. contents::

Introduction
------------

The distributed adjacency list implements a graph data structure using
an adjacency list representation. Its interface and behavior are
roughly equivalent to the Boost Graph Library's adjacency_list_
class template. However, the distributed adjacency list partitions the
vertices of the graph across several processes (which need not share
an address space). Edges outgoing from any vertex stored by a process
are also stored on that process, except in the case of undirected
graphs, where either the source or the target may store the edge.

::

  template<typename OutEdgeListS, typename ProcessGroup, typename VertexListS,
           typename DirectedS, typename VertexProperty, typename EdgeProperty, 
           typename GraphProperty, typename EdgeListS>
  class adjacency_list<OutEdgeListS, 
                       distributedS<ProcessGroup, VertexListS>,
                       DirectedS, VertexProperty,
                       EdgeProperty, GraphProperty, EdgeListS>;

Defining a Distributed Adjacency List
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To create a distributed adjacency list, first determine the
representation of the graph in a single address space using these
guidelines_. Next, replace the vertex list selector (``VertexListS``)
with an instantiation of distributedS_, which includes:

- Selecting a `process group`_ type that represents the various
  coordinating processes that will store the distributed graph. For
  example, the `MPI process group`_.

- A selector indicating how the vertex lists in each process will be
  stored. Only the ``vecS`` and ``listS`` selectors are well-supported
  at this time.


The following ``typedef`` defines a distributed adjacency list
containing directed edges. The vertices in the graph will be
distributed over several processes, using the MPI process group
for communication. In each process, the vertices and outgoing edges
will be stored in STL vectors. There are no properties attached to the
vertices or edges of the graph.

::

  using namespace boost;
  typedef adjacency_list<vecS, 
                         distributedS<parallel::mpi::bsp_process_group, vecS>,
                         directedS> 
    Graph;


Distributed Vertex and Edge Properties
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vertex and edge properties for distributed graphs mirror their
counterparts for non-distributed graphs. With a distributed graph,
however, vertex and edge properties are stored only in the process
that owns the vertex or edge. 

The most direct way to attach properties to a distributed graph is to
create a structure that will be attached to each of the vertices and
edges of the graph. For example, if we wish to model a simplistic map
of the United States interstate highway system, we might state that
the vertices of the graph are cities and the edges are highways, then
define the information that we maintain for each city and highway:

::

  struct City {
    City() { }
    City(const std::string& name, const std::string& mayor = "Unknown", int population = 0)
      : name(name), mayor(mayor), population(population) { }

    std::string name;
    std::string mayor;
    int population;

    // Serialization support is required!
    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int /*version*/) {
      ar & name & mayor & population;
    }
  };

  struct Highway {
    Highway() { }
    Highway(const std::string& name, int lanes, int miles_per_hour, int length) 
      : name(name), lanes(lanes), miles_per_hour(miles_per_hour), length(length) { }

    std::string name;
    int lanes;
    int miles_per_hour;
    int length;

    // Serialization support is required!
    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int /*version*/) {
      ar & name & lanes & miles_per_hour & length;
    }
  };


To create a distributed graph for a road map, we supply ``City`` and
``Highway`` as the fourth and fifth parameters to ``adjacency_list``,
respectively:

::

  typedef adjacency_list<vecS, 
                         distributedS<parallel::mpi::bsp_process_group, vecS>,
                         directedS,
                         City, Highway> 
    RoadMap;


Property maps for internal properties retain their behavior with
distributed graphs via `distributed property maps`_, which automate
communication among processes so that ``put`` and ``get`` operations
may be applied to non-local vertices and edges. However, for
distributed adjacency lists that store vertices in a vector
(``VertexListS`` is ``vecS``), the automatic ``vertex_index``
property map restricts the domain of ``put`` and ``get`` operations
to vertices local to the process executing the operation. For example,
we can create a property map that accesses the length of each highway
as follows:

::

  RoadMap map; // distributed graph instance

  typedef property_map<RoadMap, int Highway::*>::type 
    road_length = get(&Highway::length, map);


Now, one can access the length of any given road based on its edge
descriptor ``e`` with the expression ``get(road_length, e)``,
regardless of which process stores the edge ``e``. 

Named Vertices
~~~~~~~~~~~~~~

The vertices of a graph typically correspond to named entities within
the application domain. In the road map example from the previous
section, the vertices in the graph represent cities. The distributed
adjacency list supports named vertices, which provides an implicit
mapping from the names of the vertices in the application domain
(e.g., the name of a city, such as "Bloomington") to the actual vertex
descriptor stored within the distributed graph (e.g., the third vertex
on processor 1). By enabling support for named vertices, one can refer
to vertices by name when manipulating the graph. For example, one can
add a highway from Indianapolis to Chicago:

::
  
  add_edge("Indianapolis", "Chicago", Highway("I-65", 4, 65, 151), map);

The distributed adjacency list will find vertices associated with the
names "Indianapolis" and "Chicago", then add an edge between these
vertices that represents I-65. The vertices may be stored on any
processor, local or remote. 

To enable named vertices, specialize the ``internal_vertex_name``
property for the structure attached to the vertices in your
graph. ``internal_vertex_name`` contains a single member, ``type``,
which is the type of a function object that accepts a vertex property
and returns the name stored within that vertex property. In the case
of our road map, the ``name`` field contains the name of each city, so
we use the ``member`` function object from the `Boost.MultiIndex`_
library to extract the name, e.g.,

::

  namespace boost { namespace graph { 

  template<>
  struct internal_vertex_name<City>
  {
    typedef multi_index::member<City, std::string, &City::name> type;
  };
  
  } }


That's it! One can now refer to vertices by name when interacting with
the distributed adjacency list.

What happens when one uses the name of a vertex that does not exist?
For example, in our ``add_edge`` call above, what happens if the
vertex named "Indianapolis" has not yet been added to the graph? By
default, the distributed adjacency list will throw an exception if a
named vertex does not yet exist. However, one can customize this
behavior by specifying a function object that constructs an instance
of the vertex property (e.g., ``City``) from just the name of the
vertex. This customization occurs via the
``internal_vertex_constructor`` trait. For example, using the
``vertex_from_name`` template (provided with the Parallel BGL), we can
state that a ``City`` can be constructed directly from the name of the
city using its second constructor:

::

  namespace boost { namespace graph {

  template<>
  struct internal_vertex_constructor<City>
  {
    typedef vertex_from_name<City> type;
  };

  } }

Now, one can add edges by vertex name freely, without worrying about
the explicit addition of vertices. The ``mayor`` and ``population``
fields will have default values, but the graph structure will be
correct. 

Building a Distributed Graph
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once you have determined the layout of your graph type, including the
specific data structures and properties, it is time to construct an
instance of the graph by adding the appropriate vertices and
edges. Construction of distributed graphs can be slightly more
complicated than construction of normal, non-distributed graph data
structures, because one must account for both the distribution of the
vertices and edges and the multiple processes executing
concurrently. There are three main ways to construct distributed
graphs:

1. *Sequence constructors*: if your data can easily be generated by a
pair of iterators that produce (source, target) pairs, you can use the
sequence constructors to build the distributed graph in parallel. This
method is often preferred when creating benchmarks, because random
graph generators like the sorted_erdos_renyi_iterator_ create the
appropriate input sequence. Note that one must provide the same input
iterator sequence on all processes. This method has the advantage that
the sequential graph-building code is identical to the parallel
graph-building code. An example constructing a random graph this way:

  ::

    typedef boost::sorted_erdos_renyi_iterator<boost::minstd_rand, Graph> ERIter;
    boost::minstd_rand gen;
    unsigned long n = 1000000; // 1M vertices
    Graph g(ERIter(gen, n, 0.00005), ERIter(), n);

2. *Adding edges by vertex number*: if you are able to map your
vertices into values in the random [0, *n*), where *n* is the number
of vertices in the distributed graph. Use this method rather than the
sequence constructors when your algorithm cannot easily be moved into
input iterators, or if you can't replicate the input sequence. For
example, you might be reading the graph from an input file:

  ::

    Graph g(n); // initialize with the total number of vertices, n
    if (process_id(g.process_group()) == 0) {
      // Only process 0 loads the graph, which is distributed automatically
      int source, target;
      for (std::cin >> source >> target)
        add_edge(vertex(source, g), vertex(target, g), g);
    }

    // All processes synchronize at this point, then the graph is complete
    synchronize(g.process_group());

3. *Adding edges by name*: if you are using named vertices, you can
construct your graph just by calling ``add_edge`` with the names of
the source and target vertices. Be careful to make sure that each edge
is added by only one process (it doesn't matter which process it is),
otherwise you will end up with multiple edges. For example, in the
following program we read edges from the standard input of process 0,
adding those edges by name. Vertices and edges will be created and
distributed automatically.

  ::

    Graph g;
    if (process_id(g.process_group()) == 0) {
      // Only process 0 loads the graph, which is distributed automatically
      std:string source, target;
      for(std::cin >> source >> target)
        add_edge(source, target, g);
    }

    // All processes synchronize at this point, then the graph is complete
    synchronize(g.process_group());


Graph Concepts
~~~~~~~~~~~~~~

The distributed adjacency list models the Graph_ concept, and in
particular the `Distributed Graph`_ concept. It also models the
`Incidence Graph`_ and `Adjacency Graph`_ concept, but restricts the
input domain of the operations to correspond to local vertices
only. For instance, a process may only access the outgoing edges of a
vertex if that vertex is stored in that process. Undirected and
bidirectional distributed adjacency lists model the `Bidirectional
Graph`_ concept, with the same domain restrictions. Distributed
adjacency lists also model the `Mutable Graph`_ concept (with domain
restrictions; see the Reference_ section), `Property Graph`_ concept,
and `Mutable Property Graph`_ concept.

Unlike its non-distributed counterpart, the distributed adjacency
list does **not** model the `Vertex List Graph`_ or `Edge List
Graph`_ concepts, because one cannot enumerate all vertices or edges
within a distributed graph. Instead, it models the weaker
`Distributed Vertex List Graph`_ and `Distributed Edge List Graph`_
concepts, which permit access to the local edges and vertices on each
processor. Note that if all processes within the process group over
which the graph is distributed iterator over their respective vertex
or edge lists, all vertices and edges will be covered once. 

Reference
---------
Since the distributed adjacency list closely follows the
non-distributed adjacency_list_, this reference documentation
only describes where the two implementations differ.

Where Defined
~~~~~~~~~~~~~

<boost/graph/distributed/adjacency_list.hpp>

Associated Types
~~~~~~~~~~~~~~~~

::

  adjacency_list::process_group_type

The type of the process group over which the graph will be
distributed.

-----------------------------------------------------------------------------

  adjacency_list::distribution_type

The type of distribution used to partition vertices in the graph.

-----------------------------------------------------------------------------

  adjacency_list::vertex_name_type

If the graph supports named vertices, this is the type used to name
vertices. Otherwise, this type is not present within the distributed
adjacency list. 


Member Functions
~~~~~~~~~~~~~~~~

::

    adjacency_list(const ProcessGroup& pg = ProcessGroup());

    adjacency_list(const GraphProperty& g, 
                   const ProcessGroup& pg = ProcessGroup());

Construct an empty distributed adjacency list with the given process
group (or default) and graph property (or default).

-----------------------------------------------------------------------------

::

    adjacency_list(vertices_size_type n, const GraphProperty& p,
                   const ProcessGroup& pg, const Distribution& distribution);

    adjacency_list(vertices_size_type n, const ProcessGroup& pg,
                   const Distribution& distribution);

    adjacency_list(vertices_size_type n, const GraphProperty& p,
                   const ProcessGroup& pg = ProcessGroup());
      
    adjacency_list(vertices_size_type n, 
                   const ProcessGroup& pg = ProcessGroup());

Construct a distributed adjacency list with ``n`` vertices,
optionally providing the graph property, process group, or
distribution. The ``n`` vertices will be distributed via the given
(or default-constructed) distribution. This operation is collective,
requiring all processes with the process group to execute it
concurrently.

-----------------------------------------------------------------------------

::

    template <class EdgeIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   vertices_size_type n, 
                   const ProcessGroup& pg = ProcessGroup(),
                   const GraphProperty& p = GraphProperty());

    template <class EdgeIterator, class EdgePropertyIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   EdgePropertyIterator ep_iter,
                   vertices_size_type n,
                   const ProcessGroup& pg = ProcessGroup(),
                   const GraphProperty& p = GraphProperty());

    template <class EdgeIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   vertices_size_type n, 
                   const ProcessGroup& process_group,
                   const Distribution& distribution,
                   const GraphProperty& p = GraphProperty());

    template <class EdgeIterator, class EdgePropertyIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   EdgePropertyIterator ep_iter,
                   vertices_size_type n,
                   const ProcessGroup& process_group,
                   const Distribution& distribution,
                   const GraphProperty& p = GraphProperty());

Construct a distributed adjacency list with ``n`` vertices and with
edges specified in the edge list given by the range ``[first, last)``. The
``EdgeIterator`` must be a model of InputIterator_. The value type of the
``EdgeIterator`` must be a ``std::pair``, where the type in the pair is an
integer type. The integers will correspond to vertices, and they must
all fall in the range of ``[0, n)``. When provided, ``ep_iter``
refers to an edge property list ``[ep_iter, ep_iter + m)`` contains
properties for each of the edges.

This constructor is a collective operation and must be executed
concurrently by each process with the same argument list. Most
importantly, the edge lists provided to the constructor in each process
should be equivalent. The vertices will be partitioned among the
processes according to the ``distribution``, with edges placed on the
process owning the source of the edge. Note that this behavior
permits sequential graph construction code to be parallelized
automatically, regardless of the underlying distribution. 

-----------------------------------------------------------------------------

::

  void clear()

Removes all of the edges and vertices from the local graph. To
eliminate all vertices and edges from the graph, this routine must be
executed in all processes.

-----------------------------------------------------------------------------

::

  process_group_type& process_group();
  const process_group_type& process_group() const;

Returns the process group over which this graph is distributed.

-----------------------------------------------------------------------------

::

  distribution_type&       distribution();
  const distribution_type& distribution() const;

Returns the distribution used for initial placement of vertices.

-----------------------------------------------------------------------------

::

  template<typename VertexProcessorMap>
    void redistribute(VertexProcessorMap vertex_to_processor);

Redistributes the vertices (and, therefore, the edges) of the
graph. The property map ``vertex_to_processor`` provides, for each
vertex, the process identifier indicating where the vertex should be
moved. Once this collective routine has completed, the distributed
graph will reflect the new distribution. All vertex and edge
descsriptors and internal and external property maps are invalidated
by this operation.

-----------------------------------------------------------------------------

::

  template<typename OStreamConstructibleArchive>
    void save(std::string const& filename) const;

  template<typename IStreamConstructibleArchive>
    void load(std::string const& filename);

Serializes the graph to a Boost.Serialization archive. 
``OStreamConstructibleArchive`` and ``IStreamConstructibleArchive``
are models of Boost.Serialization *Archive* with the extra
requirement that they can be constructed from a ``std::ostream`` 
and ``std::istream``.
``filename`` names a directory that will hold files for
the different processes.


Non-Member Functions
~~~~~~~~~~~~~~~~~~~~

::

  std::pair<vertex_iterator, vertex_iterator>
  vertices(const adjacency_list& g);

Returns an iterator-range providing access to the vertex set of graph
``g`` stored in this process. Each of the processes that contain ``g``
will get disjoint sets of vertices.

-----------------------------------------------------------------------------

::

  std::pair<edge_iterator, edge_iterator>
  edges(const adjacency_list& g);

Returns an iterator-range providing access to the edge set of graph
``g`` stored in this process.

-----------------------------------------------------------------------------

::

  std::pair<adjacency_iterator, adjacency_iterator>
  adjacent_vertices(vertex_descriptor u, const adjacency_list& g);

Returns an iterator-range providing access to the vertices adjacent to
vertex ``u`` in graph ``g``. The vertex ``u`` must be local to this process.

-----------------------------------------------------------------------------

::

  std::pair<out_edge_iterator, out_edge_iterator>
  out_edges(vertex_descriptor u, const adjacency_list& g);

Returns an iterator-range providing access to the out-edges of vertex
``u`` in graph ``g``. If the graph is undirected, this iterator-range
provides access to all edges incident on vertex ``u``. For both
directed and undirected graphs, for an out-edge ``e``, ``source(e, g)
== u`` and ``target(e, g) == v`` where ``v`` is a vertex adjacent to
``u``. The vertex ``u`` must be local to this process.

-----------------------------------------------------------------------------

::

  std::pair<in_edge_iterator, in_edge_iterator>
  in_edges(vertex_descriptor v, const adjacency_list& g);

Returns an iterator-range providing access to the in-edges of vertex
``v`` in graph ``g``. This operation is only available if
``bidirectionalS`` was specified for the ``Directed`` template
parameter. For an in-edge ``e``, ``target(e, g) == v`` and ``source(e,
g) == u`` for some vertex ``u`` that is adjacent to ``v``, whether the
graph is directed or undirected. The vertex ``v`` must be local to
this process.

-----------------------------------------------------------------------------

::

  degree_size_type
  out_degree(vertex_descriptor u, const adjacency_list& g);

Returns the number of edges leaving vertex ``u``. Vertex ``u`` must
be local to the executing process.

-----------------------------------------------------------------------------

::

  degree_size_type
  in_degree(vertex_descriptor u, const adjacency_list& g);

Returns the number of edges entering vertex ``u``. This operation is
only available if ``bidirectionalS`` was specified for the
``Directed`` template parameter. Vertex ``u`` must be local to the
executing process.

-----------------------------------------------------------------------------

::

  vertices_size_type
  num_vertices(const adjacency_list& g);

Returns the number of vertices in the graph ``g`` that are stored in
the executing process.

-----------------------------------------------------------------------------

::

  edges_size_type
  num_edges(const adjacency_list& g);

Returns the number of edges in the graph ``g`` that are stored in the
executing process.

-----------------------------------------------------------------------------

::

  vertex_descriptor
  vertex(vertices_size_type n, const adjacency_list& g);

Returns the ``n``th vertex in the graph's vertex list, according to the
distribution used to partition the graph. ``n`` must be a value
between zero and the sum of ``num_vertices(g)`` in each process (minus
one). Note that it is not necessarily the case that ``vertex(0, g) ==
*num_vertices(g).first``. This function is only guaranteed to be
accurate when no vertices have been added to or removed from the
graph after its initial construction.

-----------------------------------------------------------------------------

::

  std::pair<edge_descriptor, bool>
  edge(vertex_descriptor u, vertex_descriptor v,
       const adjacency_list& g);

Returns an edge connecting vertex ``u`` to vertex ``v`` in graph
``g``. For bidirectional and undirected graphs, either vertex ``u`` or
vertex ``v`` must be local; for directed graphs, vertex ``u`` must be
local.

-----------------------------------------------------------------------------

::

  std::pair<out_edge_iterator, out_edge_iterator>
  edge_range(vertex_descriptor u, vertex_descriptor v,
             const adjacency_list& g);

TODO: Not implemented.  Returns a pair of out-edge iterators that give
the range for all the parallel edges from ``u`` to ``v``. This function only
works when the ``OutEdgeList`` for the ``adjacency_list`` is a container that
sorts the out edges according to target vertex, and allows for
parallel edges. The ``multisetS`` selector chooses such a
container. Vertex ``u`` must be stored in the executing process.

Structure Modification
~~~~~~~~~~~~~~~~~~~~~~

::

  unspecified add_edge(vertex_descriptor u, vertex_descriptor v, adjacency_list& g);
  unspecified add_edge(vertex_name_type u, vertex_descriptor v, adjacency_list& g);
  unspecified add_edge(vertex_descriptor u, vertex_name_type v, adjacency_list& g);
  unspecified add_edge(vertex_name_type u, vertex_name_type v, adjacency_list& g);

Adds edge ``(u,v)`` to the graph. The return type itself is
unspecified, but the type can be copy-constructed and implicitly
converted into a ``std::pair<edge_descriptor,bool>``. The edge
descriptor describes the added (or found) edge. For graphs that do not
allow parallel edges, if the edge 
is already in the graph then a duplicate will not be added and the
``bool`` flag will be ``false``. Also, if u and v are descriptors for
the same vertex (creating a self loop) and the graph is undirected,
then the edge will not be added and the flag will be ``false``. When
the flag is ``false``, the returned edge descriptor points to the
already existing edge. 

The parameters ``u`` and ``v`` can be either vertex descriptors or, if
the graph uses named vertices, the names of vertices. If no vertex
with the given name exists, the internal vertex constructor will be
invoked to create a new vertex property and a vertex with that
property will be added to the graph implicitly. The default internal
vertex constructor throws an exception.

-----------------------------------------------------------------------------

::

  unspecified add_edge(vertex_descriptor u, vertex_descriptor v, const EdgeProperties& p, adjacency_list& g);
  unspecified add_edge(vertex_name_type u, vertex_descriptor v, const EdgeProperties& p, adjacency_list& g);
  unspecified add_edge(vertex_descriptor u, vertex_name_type v, const EdgeProperties& p, adjacency_list& g);
  unspecified add_edge(vertex_name_type u, vertex_name_type v, const EdgeProperties& p, adjacency_list& g);


Adds edge ``(u,v)`` to the graph and attaches ``p`` as the value of the edge's
internal property storage. See the previous ``add_edge()`` member
function for more details.  

-----------------------------------------------------------------------------

::

  void 
  remove_edge(vertex_descriptor u, vertex_descriptor v, 
              adjacency_list& g);

Removes the edge ``(u,v)`` from the graph. If the directed selector is
``bidirectionalS`` or ``undirectedS``, either the source or target
vertex of the graph must be local. If the directed selector is
``directedS``, then the source vertex must be local.

-----------------------------------------------------------------------------

::

  void 
  remove_edge(edge_descriptor e, adjacency_list& g);

Removes the edge ``e`` from the graph. If the directed selector is
``bidirectionalS`` or ``undirectedS``, either the source or target
vertex of the graph must be local. If the directed selector is
``directedS``, then the source vertex must be local.

-----------------------------------------------------------------------------

::

  void remove_edge(out_edge_iterator iter, adjacency_list& g);

This has the same effect as ``remove_edge(*iter, g)``. For directed
(but not bidirectional) graphs, this will be more efficient than
``remove_edge(*iter, g)``.

-----------------------------------------------------------------------------

::

  template <class Predicate >
  void remove_out_edge_if(vertex_descriptor u, Predicate predicate,
                          adjacency_list& g);

Removes all out-edges of vertex ``u`` from the graph that satisfy the
predicate. That is, if the predicate returns ``true`` when applied to an
edge descriptor, then the edge is removed. The vertex ``u`` must be local.

The affect on descriptor and iterator stability is the same as that of
invoking remove_edge() on each of the removed edges.

-----------------------------------------------------------------------------

::

  template <class Predicate >
  void remove_in_edge_if(vertex_descriptor v, Predicate predicate,
                         adjacency_list& g);

Removes all in-edges of vertex ``v`` from the graph that satisfy the
predicate. That is, if the predicate returns true when applied to an
edge descriptor, then the edge is removed. The vertex ``v`` must be local.

The affect on descriptor and iterator stability is the same as that of
invoking ``remove_edge()`` on each of the removed edges.

This operation is available for undirected and bidirectional
adjacency_list graphs, but not for directed.  

-----------------------------------------------------------------------------

::

  template <class Predicate> 
  void remove_edge_if(Predicate predicate, adjacency_list& g);

Removes all edges from the graph that satisfy the predicate. That is,
if the predicate returns true when applied to an edge descriptor, then
the edge is removed. 

The affect on descriptor and iterator stability is the same as that
of invoking ``remove_edge()`` on each of the removed edges.

-----------------------------------------------------------------------------

::

  vertex_descriptor add_vertex(adjacency_list& g);

Adds a vertex to the graph and returns the vertex descriptor for the
new vertex. The vertex will be stored in the local process. This
function is not available when using named vertices.

-----------------------------------------------------------------------------

::

  unspecified add_vertex(const VertexProperties& p, adjacency_list& g);
  unspecified add_vertex(const vertex_name_type& p, adjacency_list& g);

Adds a vertex to the graph with the specified properties. If the graph
is using vertex names, the vertex will be added on whichever process
"owns" that name. Otherwise, the vertex will be stored in the local
process. Note that the second constructor will invoke the
user-customizable internal vertex constructor, which (by default)
throws an exception when it sees an unknown vertex. 

The return type is of unspecified type, but can be copy-constructed
and can be implicitly converted into a vertex descriptor.

-----------------------------------------------------------------------------

::

  void clear_vertex(vertex_descriptor u, adjacency_list& g);

Removes all edges to and from vertex ``u``. The vertex still appears
in the vertex set of the graph.

The affect on descriptor and iterator stability is the same as that of
invoking ``remove_edge()`` for all of the edges that have ``u`` as the source
or target.

This operation is not applicable to directed graphs, because the
incoming edges to vertex ``u`` are not known.

-----------------------------------------------------------------------------

::

  void clear_out_edges(vertex_descriptor u, adjacency_list& g);

Removes all out-edges from vertex ``u``. The vertex still appears in
the vertex set of the graph. 

The affect on descriptor and iterator stability is the same as that of
invoking ``remove_edge()`` for all of the edges that have ``u`` as the
source. 

This operation is not applicable to undirected graphs (use
``clear_vertex()`` instead).

-----------------------------------------------------------------------------

::

  void clear_in_edges(vertex_descriptor u, adjacency_list& g);

Removes all in-edges from vertex ``u``. The vertex still appears in
the vertex set of the graph.

The affect on descriptor and iterator stability is the same as that of
invoking ``remove_edge()`` for all of the edges that have ``u`` as the
target. 

This operation is only applicable to bidirectional graphs.

-----------------------------------------------------------------------------

::

  void remove_vertex(vertex_descriptor u, adjacency_list& g);

Remove vertex ``u`` from the vertex set of the graph. It is assumed
that there are no edges to or from vertex ``u`` when it is
removed. One way to make sure of this is to invoke ``clear_vertex()``
beforehand. The vertex ``u`` must be stored locally.

Property Map Accessors
~~~~~~~~~~~~~~~~~~~~~~

::

  template <class PropertyTag>
  property_map<adjacency_list, PropertyTag>::type
  get(PropertyTag, adjacency_list& g);

  template <class PropertyTag>
  property_map<adjacency_list, Tag>::const_type
  get(PropertyTag, const adjacency_list& g);

Returns the property map object for the vertex property specified by
``PropertyTag``. The ``PropertyTag`` must match one of the properties
specified in the graph's ``VertexProperty`` template argument. The
returned property map will be a `distributed property map`_.

-----------------------------------------------------------------------------

::

  template <class PropertyTag , class X>
  typename property_traits<property_map<adjacency_list, PropertyTag>::const_type>::value_type
  get(PropertyTag, const adjacency_list& g, X x);

This returns the property value for ``x``, where ``x`` is either a vertex or
edge descriptor.  The entity referred to by descriptor ``x`` must be
stored in the local process.

-----------------------------------------------------------------------------

::

  template <class PropertyTag , class X, class Value>
  void put(PropertyTag, const adjacency_list& g, X x, const Value& value);

This sets the property value for ``x`` to value. ``x`` is either a
vertex or edge descriptor. ``Value`` must be convertible to ``typename
property_traits<property_map<adjacency_list,
PropertyTag>::type>::value_type``. 

-----------------------------------------------------------------------------

::

  template <class GraphProperties, class GraphPropertyTag>
  typename graph_property<adjacency_list, GraphPropertyTag>::type&
  get_property(adjacency_list& g, GraphPropertyTag);

  template <class GraphProperties, class GraphPropertyTag >
  const typename graph_property<adjacency_list, GraphPropertyTag>::type&
  get_property(const adjacency_list& g, GraphPropertyTag);

TODO: not implemented.

Return the property specified by ``GraphPropertyTag`` that is attached
to the graph object ``g``. The ``graph_property`` traits class is
defined in ``boost/graph/adjacency_list.hpp``.

-----------------------------------------------------------------------------

Copyright (C) 2004 The Trustees of Indiana University.

Copyright (C) 2007 Douglas Gregor

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _adjacency_list: http://www.boost.org/libs/graph/doc/adjacency_list.html
.. _guidelines: http://www.boost.org/libs/graph/doc/using_adjacency_list.html
.. _process group: process_group.html
.. _mpi process group: process_group.html
.. _distributedS: distributedS.html
.. _Graph: http://www.boost.org/libs/graph/doc/Graph.html
.. _Distributed graph: DistributedGraph.html
.. _Incidence Graph: http://www.boost.org/libs/graph/doc/IncidenceGraph.html
.. _Adjacency Graph: http://www.boost.org/libs/graph/doc/AdjacencyGraph.html
.. _Bidirectional Graph: http://www.boost.org/libs/graph/doc/BidirectionalGraph.html
.. _Mutable Graph: http://www.boost.org/libs/graph/doc/MutableGraph.html
.. _Property Graph: http://www.boost.org/libs/graph/doc/PropertyGraph.html
.. _Mutable Property Graph: http://www.boost.org/libs/graph/doc/MutablePropertyGraph.html
.. _Vertex List Graph: http://www.boost.org/libs/graph/doc/VertexListGraph.html
.. _Edge List Graph: http://www.boost.org/libs/graph/doc/EdgeListGraph.html
.. _Distribution: concepts/Distribution.html
.. _distributed property map: distributed_property_map.html
.. _distributed property maps: distributed_property_map.html
.. _Distributed Vertex List Graph: DistributedVertexListGraph.html
.. _Distributed Edge List Graph: DistributedEdgeListGraph.html
.. _InputIterator: http://www.boost.org/doc/html/InputIterator.html
.. _member: http://www.boost.org/libs/multi_index/doc/reference/key_extraction.html#member_synopsis
.. _Boost.MultiIndex: http://www.boost.org/libs/multi_index/doc/index.html
.. _sorted_erdos_renyi_iterator: http://www.boost.org/libs/graph/doc/sorted_erdos_renyi_gen.html
