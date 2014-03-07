.. Iterators/Concepts//Random Access Iterator |30

Random Access Iterator
======================

Description
-----------

A |Random Access Iterator| is a |Bidirectional Iterator| that provides 
constant-time guarantees on moving the iterator an arbitrary number of positions
forward or backward and for measuring the distance to another iterator in the 
same sequence.

Refinement of
-------------

|Bidirectional Iterator|


Expression requirements
-----------------------

In addition to the requirements defined in |Bidirectional Iterator|, 
the following requirements must be met. 

+---------------------------+-------------------------------------------+---------------------------+
| Expression                | Type                                      | Complexity                |
+===========================+===========================================+===========================+
| ``next<i>::type``         | |Random Access Iterator|                  | Amortized constant time   |
+---------------------------+-------------------------------------------+---------------------------+
| ``prior<i>::type``        | |Random Access Iterator|                  | Amortized constant time   |
+---------------------------+-------------------------------------------+---------------------------+
| ``i::category``           | |Integral Constant|, convertible          | Constant time             |
|                           | to ``random_access_iterator_tag``         |                           |
+---------------------------+-------------------------------------------+---------------------------+
| ``advance<i,n>::type``    | |Random Access Iterator|                  | Amortized constant time   |
+---------------------------+-------------------------------------------+---------------------------+
| ``distance<i,j>::type``   | |Integral Constant|                       | Amortized constant time   |
+---------------------------+-------------------------------------------+---------------------------+


Expression semantics
--------------------

.. parsed-literal::

    typedef advance<i,n>::type j;

:Semantics:
    See ``advance`` specification
    

.. ..........................................................................

.. parsed-literal::

    typedef distance<i,j>::type n;

:Semantics:
    See ``distance`` specification


Invariants
----------

For any random access iterators ``i`` and ``j`` the following invariants always 
hold: 

* If ``advance<i,n>::type`` is well-defined, then 
  ``advance< advance<i,n>::type, negate<n>::type >::type`` is a null operation. 


See also
--------

|Iterators|, |Bidirectional Iterator|, |Random Access Sequence|, |advance|, |distance|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
