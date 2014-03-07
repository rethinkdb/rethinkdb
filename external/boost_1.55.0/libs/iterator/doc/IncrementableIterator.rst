.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Incrementable Iterator Concept
..............................

A class or built-in type ``X`` models the *Incrementable Iterator*
concept if, in addition to ``X`` being Assignable and Copy
Constructible, the following expressions are valid and respect the
stated semantics.


+-------------------------------------------------------------------------------------+
|Incrementable Iterator Requirements (in addition to Assignable, Copy Constructible)  |
|                                                                                     |
+--------------------------------+-------------------------------+--------------------+
|Expression                      |Return Type                    |Assertion/Semantics |
+================================+===============================+====================+
|``++r``                         |``X&``                         |``&r == &++r``      |
+--------------------------------+-------------------------------+--------------------+
|``r++``                         |``X``                          |::                  |
|                                |                               |                    |
|                                |                               | {                  |
|                                |                               |    X tmp = r;      |
|                                |                               |    ++r;            |
|                                |                               |    return tmp;     |
|                                |                               | }                  |
+--------------------------------+-------------------------------+--------------------+
|``iterator_traversal<X>::type`` |Convertible to                 |                    |
|                                |``incrementable_traversal_tag``|                    |
+--------------------------------+-------------------------------+--------------------+
