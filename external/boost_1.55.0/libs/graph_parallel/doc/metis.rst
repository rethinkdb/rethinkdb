.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

=========================================
|Logo| METIS Input Routines
=========================================

:: 

  namespace boost { 
    namespace graph {
      class metis_reader;
      class metis_exception;
      class metis_input_exception;
      class metis_distribution;
    }
  }


METIS_ is a set of programs for partitioning graphs (among other
things). The Parallel BGL can read the METIS graph format and
partition format, allowing one to easily load METIS-partitioned
graphs into the Parallel BGL's data structures.

.. contents::

Where Defined
~~~~~~~~~~~~~
<``boost/graph/metis.hpp``>


Graph Reader
------------------

::

  class metis_reader
  {
   public:
    typedef std::size_t vertices_size_type;
    typedef std::size_t edges_size_type;
    typedef double vertex_weight_type;
    typedef double edge_weight_type;

    class edge_iterator;
    class edge_weight_iterator;
    
    metis_reader(std::istream& in);

    edge_iterator begin();
    edge_iterator end();
    edge_weight_iterator weight_begin();

    vertices_size_type num_vertices() const;
    edges_size_type num_edges() const;

    std::size_t num_vertex_weights() const;

    vertex_weight_type vertex_weight(vertices_size_type v, std::size_t n);

    bool has_edge_weights() const;
  };


Usage
~~~~~

The METIS reader provides an iterator interface to the METIS graph
file. The iterator interface is most useful when constructing Parallel
BGL graphs on-the-fly. For instance, the following code builds a graph
``g`` from a METIS graph stored in ``argv[1]``. 

::

  std::ifstream in_graph(argv[1]);
  metis_reader reader(in_graph);
  Graph g(reader.begin(), reader.end(),
          reader.weight_begin(),
          reader.num_vertices());


The calls to ``begin()`` and ``end()`` return an iterator range for
the edges in the graph; the call to ``weight_begin()`` returns an
iterator that will enumerate the weights of the edges in the
graph. For a distributed graph, the distribution will be determined
automatically by the graph; to use a METIS partitioning, see the
section `Partition Reader`_.

Associated Types 
~~~~~~~~~~~~~~~~

::

  metis_reader::edge_iterator

An `Input Iterator`_ that enumerates the edges in the METIS graph, and
is suitable for use as the ``EdgeIterator`` of an adjacency_list_.
The ``value_type`` of this iterator is a pair of vertex numbers.

-----------------------------------------------------------------------------

::

  metis_reader::edge_weight_iterator

An `Input Iterator`_ that enumerates the edge weights in the METIS
graph. The ``value_type`` of this iterator is ``edge_weight_type``. If
the edges in the METIS graph are unweighted, the result of
dereferencing this iterator will always be zero.

Member Functions
~~~~~~~~~~~~~~~~

::

  metis_reader(std::istream& in);

Constructs a new METIS reader that will retrieve edges from the input
stream ``in``. If any errors are encountered while initially parsing
``in``, ``metis_input_exception`` will be thrown.

-----------------------------------------------------------------------------

::

  edge_iterator begin();

Returns an iterator to the first edge in the METIS file. 

-----------------------------------------------------------------------------

::

  edge_iterator end();

Returns an iterator one past the last edge in the METIS file.

-----------------------------------------------------------------------------

::

  edge_weight_iterator weight_begin();

Returns an iterator to the first edge weight in the METIS file. The
weight iterator should be moved in concert with the edge iterator;
when the edge iterator moves, the edge weight changes. If the edges
in the graph are unweighted, the weight returned will always be zero.

-----------------------------------------------------------------------------

::

  vertices_size_type num_vertices() const;

Returns the number of vertices in the graph.


-----------------------------------------------------------------------------

::

    edges_size_type num_edges() const;

Returns the number of edges in the graph.

-----------------------------------------------------------------------------

::

    std::size_t num_vertex_weights() const;

Returns the number of weights attached to each vertex.

-----------------------------------------------------------------------------

::

    vertex_weight_type vertex_weight(vertices_size_type v, std::size_t n);

-----------------------------------------------------------------------------

::

    bool has_edge_weights() const;  

Returns ``true`` when the edges of the graph have weights, ``false``
otherwise. When ``false``, the edge weight iterator is still valid
but returns zero for the weight of each edge.


Partition Reader
----------------

::

  class metis_distribution
  {
   public:  
    typedef int process_id_type;
    typedef std::size_t size_type;

    metis_distribution(std::istream& in, process_id_type my_id);
    
    size_type block_size(process_id_type id, size_type) const;
    process_id_type operator()(size_type n);
    size_type local(size_type n) const;
    size_type global(size_type n) const;
    size_type global(process_id_type id, size_type n) const;

   private:
    std::istream& in;
    process_id_type my_id;
    std::vector<process_id_type> vertices;
  };


Usage
~~~~~

The class ``metis_distribution`` loads a METIS partition file and
makes it available as a Distribution suitable for use with the
`distributed adjacency list`_ graph type. To load a METIS graph using
a METIS partitioning, use a ``metis_reader`` object for the graph and
a ``metis_distribution`` object for the distribution, as in the
following example.

::

  std::ifstream in_graph(argv[1]);
  metis_reader reader(in_graph);

  std::ifstream in_partitions(argv[2]);
  metis_distribution dist(in_partitions, process_id(pg));
  Graph g(reader.begin(), reader.end(),
          reader.weight_begin(),
          reader.num_vertices(),
          pg,
          dist);

In this example, ``argv[1]`` is the graph and ``argv[2]`` is the
partition file generated by ``pmetis``. The ``dist`` object loads the
partitioning information from the input stream it is given and uses
that to distributed the adjacency list. Note that the input stream
must be in the METIS partition file format and must have been
partitioned for the same number of processes are there are in the
process group ``pg``.

Member Functions
~~~~~~~~~~~~~~~~

::

  metis_distribution(std::istream& in, process_id_type my_id);

Creates a new METIS distribution from the input stream
``in``. ``my_id`` is the process ID of the current process in the
process group over which the graph will be distributed.

-----------------------------------------------------------------------------

::

  size_type block_size(process_id_type id, size_type) const;

Returns the number of vertices to be stored in the process
``id``. The second parameter, ``size_type``, is unused and may be any
value. 

-----------------------------------------------------------------------------

::

  process_id_type operator()(size_type n);

Returns the ID for the process that will store vertex number ``n``.

-----------------------------------------------------------------------------

::

  size_type local(size_type n) const;

Returns the local index of vertex number ``n`` within its owning
process. 

-----------------------------------------------------------------------------

::

  size_type global(size_type n) const;

Returns the global index of the current processor's local vertex ``n``. 

-----------------------------------------------------------------------------


::

  size_type global(process_id_type id, size_type n) const;
  
Returns the global index of the process ``id``'s local vertex ``n``.

-----------------------------------------------------------------------------

Copyright (C) 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. _METIS: http://www-users.cs.umn.edu/~karypis/metis/metis/
.. _distributed adjacency list: distributed_adjacency_list.html
.. _adjacency_list: http://www.boost.org/libs/graph/doc/adjacency_list.html
.. _input iterator: http://www.sgi.com/tech/stl/InputIterator.html

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl
          
