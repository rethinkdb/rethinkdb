.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

======================================
``distributedS`` Distribution Selector
======================================

The Boost Graph Library's class template adjacency_list_ supports
several selectors that indicate what data structure should be used for
the storage of edges or vertices. The selector ``vecS``, for instance,
indicates storage in a ``std::vector`` whereas ``listS`` indicates
storage in a ``std::list``. The Parallel BGL's `distributed
adjacency list`_ supports an additional selector, ``distributedS``,
that indicates that the storage should be distributed across multiple
processes. This selector can transform a sequential adjacency list
into a distributed adjacency list.

::

  template<typename ProcessGroup, typename LocalSelector = vecS>
  struct distributedS;


Template parameters
~~~~~~~~~~~~~~~~~~~

**ProcessGroup**:
  The type of the process group over which the property map is
  distributed and also the medium for communication. This type must
  model the `Process Group`_ concept, but certain data structures may
  place additional requirements on this parameter.

**LocalSelector**:
  A selector type (e.g., ``vecS``) that indicates how vertices or
  edges should be stored in each process. 

-----------------------------------------------------------------------------

Copyright (C) 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine


.. _adjacency_list: http://www.boost.org/libs/graph/doc/adjacency_list.html
.. _Distributed adjacency list: distributed_adjacency_list.html
.. _Process group: process_group.html

