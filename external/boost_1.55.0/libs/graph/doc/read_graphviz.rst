============================
|(logo)|__ ``read_graphviz``
============================

.. |(logo)| image:: ../../../boost.png
   :align: middle
   :alt: Boost

.. Copyright (c) 2005-2009 Trustees of Indiana University
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at
     http://www.boost.org/LICENSE_1_0.txt)
__ ../../../index.htm

::

  namespace boost {
  
    template <typename MutableGraph>
    bool read_graphviz(std::istream& in, MutableGraph& graph,
                       dynamic_properties& dp, 
                       const std::string& node_id = "node_id");
  
    template <typename MutableGraph>
    bool read_graphviz(std::string& str, MutableGraph& graph,
                       dynamic_properties& dp, 
                       const std::string& node_id = "node_id");
  
    template <typename InputIterator, typename MutableGraph>
    bool read_graphviz(InputIterator begin, InputIterator end,
                       MutableGraph& graph, dynamic_properties& dp, 
                       const std::string& node_id = "node_id");
  
  }

 
The ``read_graphviz`` function interprets a graph described using the
GraphViz_ DOT language and builds a BGL graph that captures that
description.  Using these functions, you can initialize a graph using
data stored as text.

The DOT language can specify both directed and undirected graphs, and
``read_graphviz`` differentiates between the two. One must pass
``read_graphviz`` an undirected graph when reading an undirected graph;
the same is true for directed graphs. Furthermore, ``read_graphviz``
will throw an exception if it encounters parallel edges and cannot add
them to the graph.

To handle properties expressed in the DOT language, ``read_graphviz``
takes a dynamic_properties_ object and operates on its collection of
property maps.  The reader passes all the properties encountered to
this object, using the GraphViz string keys as the property keys.
Furthermore, ``read_graphviz`` stores node identifier names under the
vertex property map named ``node_id``. 

Requirements:
 - The type of the graph must model the `Mutable Graph`_ concept.
 - The type of the iterator must model the `Input Iterator`_
   concept.
 - The property map value types must be default-constructible.


.. contents::

Where Defined
-------------
``<boost/graph/graphviz.hpp>``

Exceptions
----------

::

  struct graph_exception : public std::exception {
    virtual ~graph_exception() throw();
    virtual const char* what() const throw() = 0;
  };

  struct bad_parallel_edge : public graph_exception {
    std::string from;
    std::string to;

    bad_parallel_edge(const std::string&, const std::string&);
    virtual ~bad_parallel_edge() throw();
    const char* what() const throw();
  };

  struct directed_graph_error : public graph_exception {
    virtual ~directed_graph_error() throw();
    virtual const char* what() const throw();
  };

  struct undirected_graph_error : public graph_exception {
    virtual ~undirected_graph_error() throw();
    virtual const char* what() const throw();
  };

  struct bad_graphviz_syntax: public graph_exception {
    std::string errmsg;

    bad_graphviz_syntax(const std::string&);
    virtual ~bad_graphviz_syntax() throw();
    virtual const char* what() const throw();
  };

Under certain circumstances, ``read_graphviz`` will throw one of the
above exceptions.  The three concrete exceptions can all be caught
using the general ``graph_exception`` moniker when greater precision
is not needed.  In addition, all of the above exceptions derive from
the standard ``std::exception`` for even more generalized error
handling.

The ``bad_parallel_edge`` exception is thrown when an attempt to add a
parallel edge to the supplied MutableGraph fails.  The DOT language
supports parallel edges, but some BGL-compatible graph types do not.
One example of such a graph is ``boost::adjacency_list<setS,vecS>``,
which allows at most one edge can between any two vertices.


The ``directed_graph_error`` exception occurs when an undirected graph
type is passed to ``read_graph`` but the textual representation of the
graph is directed, as indicated by the ``digraph`` keyword in the DOT
language.

The ``undirected_graph_error`` exception occurs when a directed graph
type is passed to ``read_graph`` but the textual representation of the
graph is undirected, as indicated by the ``graph`` keyword in the DOT
language.

The ``bad_graphviz_syntax`` exception occurs when the graph input is not a
valid GraphViz graph.


Example
-------
The following example illustrates a relatively simple use of the
GraphViz reader to populate an ``adjacency_list`` graph

::

  // Vertex properties
  typedef property < vertex_name_t, std::string,
            property < vertex_color_t, float > > vertex_p;  
  // Edge properties
  typedef property < edge_weight_t, double > edge_p;
  // Graph properties
  typedef property < graph_name_t, std::string > graph_p;
  // adjacency_list-based type
  typedef adjacency_list < vecS, vecS, directedS,
    vertex_p, edge_p, graph_p > graph_t;

  // Construct an empty graph and prepare the dynamic_property_maps.
  graph_t graph(0);
  dynamic_properties dp;

  property_map<graph_t, vertex_name_t>::type name =
    get(vertex_name, graph);
  dp.property("node_id",name);

  property_map<graph_t, vertex_color_t>::type mass =
    get(vertex_color, graph);
  dp.property("mass",mass);

  property_map<graph_t, edge_weight_t>::type weight =
    get(edge_weight, graph);
  dp.property("weight",weight);

  // Use ref_property_map to turn a graph property into a property map
  boost::ref_property_map<graph_t*,std::string> 
    gname(get_property(graph,graph_name));
  dp.property("name",gname);

  // Sample graph as an std::istream;
  std::istringstream
    gvgraph("digraph { graph [name=\"graphname\"]  a  c e [mass = 6.66] }");

  bool status = read_graphviz(gvgraph,graph,dp,"node_id");




Building the GraphViz Readers
-----------------------------
To use the GraphViz readers, you will need to build and link against
the "boost_graph" and "boost_regex" libraries. These libraries can be built by following the
`Boost Jam Build Instructions`_ for the subdirectories ``libs/graph/build`` and ``libs/regex/build``.


Notes
-----

 - The ``read_graphviz`` function does not use any code from the
   GraphViz distribution to interpret the DOT Language.  Rather, the
   implementation was based on documentation found on the GraphViz web
   site, as well as experiments run using the dot application.  The
   resulting interpretation may be subtly different from dot for some
   corner cases that are not well specified.

 - On successful reading of a graph, every vertex and edge will have
   an associated value for every respective edge and vertex property
   encountered while interpreting the graph.  These values will be set
   using the ``dynamic_properties`` object.  Those edges and
   vertices that are not explicitly given a value for a property (and that
   property has no default) will be
   given the default constructed value of the value type.  **Be sure
   that property map value types are default constructible.**

 - ``read_graphviz`` treats subgraphs as syntactic sugar.  It does not
   reflect subgraphs as actual entities in the BGL.  Rather, they are
   used to shorten some edge definitions as well as to give a subset
   of all nodes or edges certain properties. For example, the
   DOT graphs ``digraph { a -> subgraph {b -> c} -> e }`` and 
   ``digraph { a -> b -> e ; a -> c -> e ; b -> c}`` are equivalent.

 - Subgraph IDs refer to subgraphs defined earlier in the graph
   description.  Undefined subgraphs behave as empty subgraphs
   (``{}``).  This is the same behavior as GraphViz.

See Also
--------

write_graphviz_


Future Work
-----------

 - Passing port information to BGL.

 - Expanding escape codes in the same way GraphViz does.

 - Support for optional recognition of subgraphs as distinct entities.


.. _GraphViz: http://graphviz.org/
.. _`Mutable Graph`: MutableGraph.html
.. _`Input Iterator`: http://www.sgi.com/tech/stl/InputIterator.html
.. _dynamic_properties: ../../property_map/doc/dynamic_property_map.html
.. _write_graphviz: write-graphviz.html
.. _Boost Jam Build Instructions: ../../../more/getting_started.html#Build_Install
