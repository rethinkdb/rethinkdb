============================
|(logo)|__ ``write_graphml``
============================

.. Copyright (C) 2006  Tiago de Paula Peixoto <tiago@forked.de>
  
   Distributed under the Boost Software License, Version 1.0. (See
   accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)
 
   Authors: Tiago de Paula Peixoto

.. |(logo)| image:: ../../../boost.png
   :align: middle
   :alt: Boost

__ ../../../index.htm

::

  template<typename Graph>
  void
  write_graphml(std::ostream& out, const Graph& g, const dynamic_properties& dp, 
	        bool ordered_vertices=false);

  template<typename Graph, typename VertexIndexMap>
  void
  write_graphml(std::ostream& out, const Graph& g, VertexIndexMap vertex_index,
                const dynamic_properties& dp, bool ordered_vertices=false);

This is to write a BGL graph object into an output stream in the
GraphML_ format.  Both overloads of ``write_graphml`` will emit all of
the properties stored in the dynamic_properties_ object, thereby
retaining the properties that have been read in through the dual
function read_graphml_. The second overload must be used when the
graph doesn't have an internal vertex index map, which must then be
supplied with the appropriate parameter.

.. contents::

Where Defined
-------------
``<boost/graph/graphml.hpp>``

Parameters
----------

OUT: ``std::ostream& out``
  A standard ``std::ostream`` object.

IN: ``VertexListGraph& g`` 
  A directed or undirected graph.  The
  graph's type must be a model of VertexListGraph_. If the graph
  doesn't have an internal ``vertex_index`` property map, one
  must be supplied with the vertex_index parameter.

IN: ``VertexIndexMap vertex_index``
  A vertex property map containing the indexes in the range
  [0,num_vertices(g)].


IN: ``dynamic_properties& dp``
  Contains all of the vertex, edge, and graph properties that should be
  emitted by the GraphML writer.

IN: ``bool ordered_vertices``
  This tells whether or not the order of the vertices from vertices(g)
  matches the order of the indexes. If ``true``, the ``parse.nodeids``
  graph attribute will be set to ``canonical``. Otherwise it will be
  set to ``free``.



Example
-------

This example demonstrates using BGL-GraphML interface to write 
a BGL graph into a GraphML format file.

::

  enum files_e { dax_h, yow_h, boz_h, zow_h, foo_cpp,
                 foo_o, bar_cpp, bar_o, libfoobar_a,
                 zig_cpp, zig_o, zag_cpp, zag_o,
                 libzigzag_a, killerapp, N };
  const char* name[] = { "dax.h", "yow.h", "boz.h", "zow.h", "foo.cpp",
                         "foo.o", "bar.cpp", "bar.o", "libfoobar.a",
                         "zig.cpp", "zig.o", "zag.cpp", "zag.o",
                         "libzigzag.a", "killerapp" };

  int main(int,char*[])
  {
      typedef pair<int,int> Edge;
      Edge used_by[] = {
          Edge(dax_h, foo_cpp), Edge(dax_h, bar_cpp), Edge(dax_h, yow_h),
          Edge(yow_h, bar_cpp), Edge(yow_h, zag_cpp),
          Edge(boz_h, bar_cpp), Edge(boz_h, zig_cpp), Edge(boz_h, zag_cpp),
          Edge(zow_h, foo_cpp),
          Edge(foo_cpp, foo_o),
          Edge(foo_o, libfoobar_a),
          Edge(bar_cpp, bar_o),
          Edge(bar_o, libfoobar_a),
          Edge(libfoobar_a, libzigzag_a),
          Edge(zig_cpp, zig_o),
          Edge(zig_o, libzigzag_a),
          Edge(zag_cpp, zag_o),
          Edge(zag_o, libzigzag_a),
          Edge(libzigzag_a, killerapp)
       };

      const int nedges = sizeof(used_by)/sizeof(Edge);

      typedef adjacency_list< vecS, vecS, directedS,
          property< vertex_color_t, string >,
          property< edge_weight_t, int >
          > Graph;
      Graph g(used_by, used_by + nedges, N);

      graph_traits<Graph>::vertex_iterator v, v_end;
      for (tie(v,v_end) = vertices(g); v != v_end; ++v)
          put(vertex_color_t(), g, *v, name[*v]);

      graph_traits<Graph>::edge_iterator e, e_end;
      for (tie(e,e_end) = edges(g); e != e_end; ++e)
          put(edge_weight_t(), g, *e, 3);

      dynamic_properties dp;
      dp.property("name", get(vertex_color_t(), g));
      dp.property("weight", get(edge_weight_t(), g));

      write_graphml(std::cout, g, dp, true);
   }


The output will be:

::

  <?xml version="1.0" encoding="UTF-8"?>
  <graphml xmlns="http://graphml.graphdrawing.org/xmlns/graphml"  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://graphml.graphdrawing.org/xmlns/graphml http://graphml.graphdrawing.org/xmlns/graphml/graphml-attributes-1.0rc.xsd">
    <key id="key0" for="node" attr.name="name" attr.type="string" />
    <key id="key1" for="edge" attr.name="weight" attr.type="int" />
    <graph id="G" edgedefault="directed" parse.nodeids="canonical" parse.edgeids="canonical" parse.order="nodesfirst">
      <node id="n0">
        <data key="key0">dax.h</data>
      </node>
      <node id="n1">
        <data key="key0">yow.h</data>
      </node>
      <node id="n2">
        <data key="key0">boz.h</data>
      </node>
      <node id="n3">
        <data key="key0">zow.h</data>
      </node>
      <node id="n4">
        <data key="key0">foo.cpp</data>
      </node>
      <node id="n5">
        <data key="key0">foo.o</data>
      </node>
      <node id="n6">
        <data key="key0">bar.cpp</data>
      </node>
      <node id="n7">
        <data key="key0">bar.o</data>
      </node>
      <node id="n8">
        <data key="key0">libfoobar.a</data>
      </node>
      <node id="n9">
        <data key="key0">zig.cpp</data>
      </node>
      <node id="n10">
        <data key="key0">zig.o</data>
      </node>
      <node id="n11">
        <data key="key0">zag.cpp</data>
      </node>
      <node id="n12">
        <data key="key0">zag.o</data>
      </node>
      <node id="n13">
        <data key="key0">libzigzag.a</data>
      </node>
      <node id="n14">
        <data key="key0">killerapp</data>
      </node>
      <edge id="e0" source="n0" target="n4">
        <data key="key1">3</data>
      </edge>
      <edge id="e1" source="n0" target="n6">
        <data key="key1">3</data>
      </edge>
      <edge id="e2" source="n0" target="n1">
        <data key="key1">3</data>
      </edge>
      <edge id="e3" source="n1" target="n6">
        <data key="key1">3</data>
      </edge>
      <edge id="e4" source="n1" target="n11">
        <data key="key1">3</data>
      </edge>
      <edge id="e5" source="n2" target="n6">
        <data key="key1">3</data>
      </edge>
      <edge id="e6" source="n2" target="n9">
	 <data key="key1">3</data>
      </edge>
      <edge id="e7" source="n2" target="n11">
	<data key="key1">3</data>
      </edge>
      <edge id="e8" source="n3" target="n4">
	<data key="key1">3</data>
      </edge>
      <edge id="e9" source="n4" target="n5">
	<data key="key1">3</data>
      </edge>
      <edge id="e10" source="n5" target="n8">
	<data key="key1">3</data>
      </edge>
      <edge id="e11" source="n6" target="n7">
	<data key="key1">3</data>
      </edge>
      <edge id="e12" source="n7" target="n8">
	<data key="key1">3</data>
      </edge>
      <edge id="e13" source="n8" target="n13">
	<data key="key1">3</data>
      </edge>
      <edge id="e14" source="n9" target="n10">
	<data key="key1">3</data>
      </edge>
      <edge id="e15" source="n10" target="n13">
	<data key="key1">3</data>
      </edge>
      <edge id="e16" source="n11" target="n12">
	<data key="key1">3</data>
      </edge>
      <edge id="e17" source="n12" target="n13">
	<data key="key1">3</data>
      </edge>
      <edge id="e18" source="n13" target="n14">
	<data key="key1">3</data>
      </edge>
    </graph>
  </graphml>

See Also
--------

_read_graphml

Notes
-----

 - Note that you can use GraphML file write facilities without linking
   against the ``boost_graph`` library.

.. _GraphML: http://graphml.graphdrawing.org/
.. _dynamic_properties: ../../property_map/doc/dynamic_property_map.html
.. _read_graphml: read_graphml.html
.. _VertexListGraph: VertexListGraph.html
