.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Interoperable Iterator Concept
..............................

A class or built-in type ``X`` that models Single Pass Iterator is
*interoperable with* a class or built-in type ``Y`` that also models
Single Pass Iterator if the following expressions are valid and
respect the stated semantics. In the tables below, ``x`` is an object
of type ``X``, ``y`` is an object of type ``Y``, ``Distance`` is
``iterator_traits<Y>::difference_type``, and ``n`` represents a
constant object of type ``Distance``.

+-----------+-----------------------+---------------------------------------------------+
|Expression |Return Type            |Assertion/Precondition/Postcondition               |
+===========+=======================+===================================================+
|``y = x``  |``Y``                  |post: ``y == x``                                   |
+-----------+-----------------------+---------------------------------------------------+
|``Y(x)``   |``Y``                  |post: ``Y(x) == x``                                |
+-----------+-----------------------+---------------------------------------------------+
|``x == y`` |convertible to ``bool``|``==`` is an equivalence relation over its domain. |
+-----------+-----------------------+---------------------------------------------------+
|``y == x`` |convertible to ``bool``|``==`` is an equivalence relation over its domain. |
+-----------+-----------------------+---------------------------------------------------+
|``x != y`` |convertible to ``bool``|``bool(a==b) != bool(a!=b)`` over its domain.      |
+-----------+-----------------------+---------------------------------------------------+
|``y != x`` |convertible to ``bool``|``bool(a==b) != bool(a!=b)`` over its domain.      |
+-----------+-----------------------+---------------------------------------------------+

If ``X`` and ``Y`` both model Random Access Traversal Iterator then
the following additional requirements must be met.

+-----------+-----------------------+---------------------+--------------------------------------+
|Expression |Return Type            |Operational Semantics|Assertion/ Precondition               |
+===========+=======================+=====================+======================================+
|``x < y``  |convertible to ``bool``|``y - x > 0``        |``<`` is a total ordering relation    |
+-----------+-----------------------+---------------------+--------------------------------------+
|``y < x``  |convertible to ``bool``|``x - y > 0``        |``<`` is a total ordering relation    |
+-----------+-----------------------+---------------------+--------------------------------------+
|``x > y``  |convertible to ``bool``|``y < x``            |``>`` is a total ordering relation    |
+-----------+-----------------------+---------------------+--------------------------------------+
|``y > x``  |convertible to ``bool``|``x < y``            |``>`` is a total ordering relation    |
+-----------+-----------------------+---------------------+--------------------------------------+
|``x >= y`` |convertible to ``bool``|``!(x < y)``         |                                      |
+-----------+-----------------------+---------------------+--------------------------------------+
|``y >= x`` |convertible to ``bool``|``!(y < x)``         |                                      |
+-----------+-----------------------+---------------------+--------------------------------------+
|``x <= y`` |convertible to ``bool``|``!(x > y)``         |                                      |
+-----------+-----------------------+---------------------+--------------------------------------+
|``y <= x`` |convertible to ``bool``|``!(y > x)``         |                                      |
+-----------+-----------------------+---------------------+--------------------------------------+
|``y - x``  |``Distance``           |``distance(Y(x),y)`` |pre: there exists a value ``n`` of    |
|           |                       |                     |``Distance`` such that ``x + n == y``.|
|           |                       |                     |``y == x + (y - x)``.                 |
+-----------+-----------------------+---------------------+--------------------------------------+ 
|``x - y``  |``Distance``           |``distance(y,Y(x))`` |pre: there exists a value ``n`` of    |
|           |                       |                     |``Distance`` such that ``y + n == x``.|
|           |                       |                     |``x == y + (x - y)``.                 |
+-----------+-----------------------+---------------------+--------------------------------------+
