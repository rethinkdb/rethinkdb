.. Copyright (C) 2004-2009 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===================================
|Logo| Scalable R-MAT generator
===================================

::
 
  template<typename ProcessGroup, typename Distribution, 
           typename RandomGenerator, typename Graph>
  class scalable_rmat_iterator
  {
  public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::pair<vertices_size_type, vertices_size_type> value_type;
    typedef const value_type& reference;
    typedef const value_type* pointer;
    typedef void difference_type;

    scalable_rmat_iterator();
    scalable_rmat_iterator(ProcessGroup pg, Distribution distrib,
                           RandomGenerator& gen, vertices_size_type n, 
                           edges_size_type m, double a, double b, double c, 
                           double d, bool permute_vertices = true);

    // Iterator operations
    reference operator*() const;
    pointer operator->() const;
    scalable_rmat_iterator& operator++();
    scalable_rmat_iterator operator++(int);
    bool operator==(const scalable_rmat_iterator& other) const;
    bool operator!=(const scalable_rmat_iterator& other) const;
 };

This class template implements a generator for R-MAT graphs [CZF04]_,
suitable for initializing an adjacency_list or other graph structure
with iterator-based initialization. An R-MAT graph has a scale-free
distribution w.r.t. vertex degree and is implemented using
Recursive-MATrix partitioning.

Where Defined
-------------
<``boost/graph/rmat_graph_generator.hpp``>

Constructors
------------

::

  scalable_rmat_iterator();

Constructs a past-the-end iterator.

::

  scalable_rmat_iterator(ProcessGroup pg, Distribution distrib,
                         RandomGenerator& gen, vertices_size_type n, 
                         edges_size_type m, double a, double b, double c, 
                         double d, bool permute_vertices = true);

Constructs an R-MAT generator iterator that creates a graph with ``n``
vertices and ``m`` edges.  Inside the ``scalable_rmat_iterator``
processes communicate using ``pg`` to generate their local edges as
defined by ``distrib``.  ``a``, ``b``, ``c``, and ``d`` represent the
probability that a generated edge is placed of each of the 4 quadrants
of the partitioned adjacency matrix.  Probabilities are drawn from the
random number generator ``gen``.  Vertex indices are permuted to
eliminate locality when ``permute_vertices`` is true.

Example
-------

::

  #include <boost/graph/distributed/mpi_process_group.hpp>
  #include <boost/graph/compressed_sparse_row_graph.hpp>
  #include <boost/graph/rmat_graph_generator.hpp>
  #include <boost/random/linear_congruential.hpp>

  using boost::graph::distributed::mpi_process_group;

  typedef compressed_sparse_row_graph<directedS, no_property, no_property, no_property,
                                      distributedS<mpi_process_group> > Graph;
  typedef boost::scalable_rmat_iterator<boost::minstd_rand, Graph> RMATGen;

  int main()
  {
    boost::minstd_rand gen;
    mpi_process_group pg;

    int N = 100;

    boost::parallel::variant_distribution<ProcessGroup> distrib 
      = boost::parallel::block(pg, N);

    // Create graph with 100 nodes and 400 edges 
    Graph g(RMATGen(pg, distrib, gen, N, 400, 0.57, 0.19, 0.19, 0.05), 
            RMATGen(), N, pg, distrib);
    return 0;
  }

Bibliography
------------

.. [CZF04] D Chakrabarti, Y Zhan, and C Faloutsos.  R-MAT: A Recursive
  Model for Graph Mining. In Proceedings of 4th International Conference
  on Data Mining, pages 442--446, 2004.

-----------------------------------------------------------------------------

Copyright (C) 2009 The Trustees of Indiana University.

Authors: Nick Edmonds, Brian Barrett, and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Sequential connected components: http://www.boost.org/libs/graph/doc/connected_components.html
