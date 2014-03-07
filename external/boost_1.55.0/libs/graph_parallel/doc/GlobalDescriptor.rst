.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

================================
|Logo| Concept Global Descriptor
================================

.. contents::

Description
-----------

A global descriptor is an object that represents an entity that is
owned by some process and may reside in an address space not
accessible to the currently-executing process. The global descriptor
consists of two parts: the *owner* of the entity, which is the
identifier of that process in which the entity resides, and a *local
descriptor*, that uniquely identifies the entity with the address
space of the owner. 

Refinement of
-------------

  - `Default Constructible`_
  - Assignable_

Notation
--------
X
  A type that models the Global Descriptor concept.

x
  Object of type X


Associated types
----------------

+----------------+--------------------+---------------------------------+
|Process ID type |``process_id_type`` |Determined by the process group  |
|                |                    |associated with type X.          |
+----------------+--------------------+---------------------------------+
|Local descriptor|``local_type``      |Determined by the data structure |
|type            |                    |the descriptor accesses.         |
|                |                    |Must model `Equality Comparable`_|
|                |                    |and `Copy Constructible`_.       |
+----------------+--------------------+---------------------------------+

Valid Expressions
-----------------

+----------------+---------------------+---------------------+-------------------------------------+
|Name            |Expression           |Type                 |Semantics                            |
+================+=====================+=====================+=====================================+
|Owner           |``owner(x)``         |``process_id_type``  |Returns the owner of ``x``.          |
+----------------+---------------------+---------------------+-------------------------------------+
|Local descriptor|``local(x)``         |``local_type``       |Returns the local descriptor         |
|                |                     |                     |uniquely identifying ``x``.          |
+----------------+---------------------+---------------------+-------------------------------------+


-----------------------------------------------------------------------------

Copyright (C) 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Assignable: http://www.sgi.com/tech/stl/Assignable.html
.. _Copy constructible: http://www.sgi.com/tech/stl/CopyConstructible.html
.. _Default constructible: http://www.sgi.com/tech/stl/DefaultConstructible.html
.. _Equality comparable: http://www.sgi.com/tech/stl/EqualityComparable.html
