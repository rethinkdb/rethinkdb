.. Copyright (C) 2004-2009 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===========================
|Logo| Fruchterman Reingold
===========================

::

    namespace graph { namespace distributed {

      template<typename Graph, typename PositionMap, 
               typename AttractiveForce, typename RepulsiveForce,
               typename ForcePairs, typename Cooling, typename DisplacementMap>
      void
      fruchterman_reingold_force_directed_layout
      (const Graph&    g,
       PositionMap     position,
       typename property_traits<PositionMap>::value_type const& origin,
       typename property_traits<PositionMap>::value_type const& extent,
       AttractiveForce attractive_force,
       RepulsiveForce  repulsive_force,
       ForcePairs      force_pairs,
       Cooling         cool,
       DisplacementMap displacement)

      template<typename Graph, typename PositionMap, 
               typename AttractiveForce, typename RepulsiveForce,
               typename ForcePairs, typename Cooling, typename DisplacementMap>
      void
      fruchterman_reingold_force_directed_layout
      (const Graph&    g,
       PositionMap     position,
       typename property_traits<PositionMap>::value_type const& origin,
       typename property_traits<PositionMap>::value_type const& extent,
       AttractiveForce attractive_force,
       RepulsiveForce  repulsive_force,
       ForcePairs      force_pairs,
       Cooling         cool,
       DisplacementMap displacement,
       simple_tiling   tiling)
    } }

.. contents::

Where Defined
-------------
<``boost/graph/distributed/fruchterman_reingold.hpp``>

also accessible from

<``boost/graph/fruchterman_reingold.hpp``>

Parameters
----------

IN:  ``const Graph& g``
  The graph type must be a model of `Distributed Graph`_.  The graph
  type must also model the `Incidence Graph`_.

OUT:  ``PositionMap position``

IN:  ``property_traits<PositionMap>::value_type origin``

IN:  ``property_traits<PositionMap>::value_type extent``

IN:  ``AttractiveForce attractive_force``

IN:  ``RepulsiveForce repulsive_force``

IN:  ``ForcePairs force_pairs``

IN:  ``Cooling cool``

IN:  ``DisplacementMap displacement``

..
 Complexity
 ----------

..
 Algorithm Description
 ---------------------

-----------------------------------------------------------------------------

Copyright (C) 2009 The Trustees of Indiana University.

Authors: Nick Edmonds and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Distributed Graph: DistributedGraph.html
.. _Incidence Graph: http://www.boost.org/libs/graph/doc/IncidenceGraph.html
.. _Distributed Property Map: distributed_property_map.html
