.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Writable Iterator Concept
.........................

A class or built-in type ``X`` models the *Writable Iterator* concept
if, in addition to ``X`` being Copy Constructible, the following
expressions are valid and respect the stated semantics.  Writable
Iterators have an associated *set of value types*.

+---------------------------------------------------------------------+
|Writable Iterator Requirements (in addition to Copy Constructible)   |
+-------------------------+--------------+----------------------------+
|Expression               |Return Type   |Precondition                |
+=========================+==============+============================+
|``*a = o``               |              | pre: The type of ``o``     |
|                         |              | is in the set of           |
|                         |              | value types of ``X``       |
+-------------------------+--------------+----------------------------+
