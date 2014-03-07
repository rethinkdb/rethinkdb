
The MPL provides a broad range of fundamental algorithms aimed to 
satisfy the majority of sequential compile-time data processing 
needs. The algorithms include compile-time counterparts
of many of the STL algorithms, iteration algorithms borrowed from 
functional programming languages, and more.

Unlike the algorithms in the C++ Standard Library, which operate on
implict *iterator ranges*, the majority of MPL counterparts take
and return *sequences*. This derivation is not dictated by the 
functional nature of C++ compile-time computations per se, but
rather by a desire to improve general usability of the library,
making programming with compile-time data structures as enjoyable 
as possible.

.. This can be seen as a further generalization and extension of 
   the STL's conceptual framework.

In the spirit of the STL, MPL algorithms are *generic*, meaning 
that they are not tied to particular sequence class 
implementations, and can operate on a wide range of arguments as 
long as they satisfy the documented requirements. The requirements
are formulated in terms of concepts. Under the hood, 
algorithms are decoupled from concrete sequence 
implementations by operating on |iterators|.

All MPL algorithms can be sorted into three 
major categories: iteration algorithms, querying algorithms, and 
transformation algorithms. The transformation algorithms introduce 
an associated |Inserter| concept, a rough equivalent for the notion of 
|Output Iterator| in the Standard Library. Moreover, every 
transformation algorithm provides a ``reverse_`` counterpart, 
allowing for a wider range of efficient transformations |--| a
common functionality documented by the |Reversible Algorithm| 
concept.


.. |Output Iterator| replace:: `Output Iterator <http://www.sgi.com/tech/stl/OutputIterator.html>`__
.. |sequence algorithms| replace:: `sequence algorithms`_
.. _`sequence algorithms`: `Algorithms`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
