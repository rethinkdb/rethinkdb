.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Forward Traversal Concept
.........................

A class or built-in type ``X`` models the *Forward Traversal*
concept if, in addition to ``X`` meeting the requirements of Default
Constructible and Single Pass Iterator, the following expressions are
valid and respect the stated semantics.

+--------------------------------------------------------------------------------------------------------+
|Forward Traversal Iterator Requirements (in addition to Default Constructible and Single Pass Iterator) |
+---------------------------------------+-----------------------------------+----------------------------+
|Expression                             |Return Type                        |Assertion/Note              |
+=======================================+===================================+============================+
|``X u;``                               |``X&``                             |note: ``u`` may have a      |
|                                       |                                   |singular value.             |
+---------------------------------------+-----------------------------------+----------------------------+
|``++r``                                |``X&``                             |``r == s`` and ``r`` is     |
|                                       |                                   |dereferenceable implies     |
|                                       |                                   |``++r == ++s.``             |
+---------------------------------------+-----------------------------------+----------------------------+
|``iterator_traits<X>::difference_type``|A signed integral type representing|                            |
|                                       |the distance between iterators     |                            |
|                                       |                                   |                            |
+---------------------------------------+-----------------------------------+----------------------------+
|``iterator_traversal<X>::type``        |Convertible to                     |                            |
|                                       |``forward_traversal_tag``          |                            |
+---------------------------------------+-----------------------------------+----------------------------+
