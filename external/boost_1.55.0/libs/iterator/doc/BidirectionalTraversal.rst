.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Bidirectional Traversal Concept
...............................

A class or built-in type ``X`` models the *Bidirectional Traversal*
concept if, in addition to ``X`` meeting the requirements of Forward
Traversal Iterator, the following expressions are valid and respect
the stated semantics.

+--------------------------------------------------------------------------------------+
|Bidirectional Traversal Iterator Requirements (in addition to Forward Traversal       |
|Iterator)                                                                             |
+--------------------------------+-------------------------------+---------------------+
|Expression                      |Return Type                    |Assertion/Semantics /|
|                                |                               |Pre-/Post-condition  |
+================================+===============================+=====================+
|``--r``                         |``X&``                         |pre: there exists    |
|                                |                               |``s`` such that ``r  |
|                                |                               |== ++s``.  post:     |
|                                |                               |``s`` is             |
|                                |                               |dereferenceable.     |
|                                |                               |``--(++r) == r``.    |
|                                |                               |``--r == --s``       |
|                                |                               |implies ``r ==       |
|                                |                               |s``. ``&r == &--r``. |
+--------------------------------+-------------------------------+---------------------+
|``r--``                         |convertible to ``const X&``    |::                   |
|                                |                               |                     |
|                                |                               | {                   |
|                                |                               |   X tmp = r;        |
|                                |                               |   --r;              |
|                                |                               |   return tmp;       |
|                                |                               | }                   |
+--------------------------------+-------------------------------+---------------------+
|``iterator_traversal<X>::type`` |Convertible to                 |                     |
|                                |``bidirectional_traversal_tag``|                     |
|                                |                               |                     |
+--------------------------------+-------------------------------+---------------------+
