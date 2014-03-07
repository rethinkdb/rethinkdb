.. Iterators/Concepts//Bidirectional Iterator |20

Bidirectional Iterator
======================

Description
-----------

A |Bidirectional Iterator| is a |Forward Iterator| that provides a way to 
obtain an iterator to the previous element in a sequence. 

Refinement of
-------------

|Forward Iterator|

Definitions
-----------

* a bidirectional iterator ``i`` is `decrementable` if there is a "previous" 
  iterator, that is, if ``prior<i>::type`` expression is well-defined; 
  iterators pointing to the first element of the sequence are not 
  decrementable. 
 

Expression requirements
-----------------------

In addition to the requirements defined in |Forward Iterator|, 
the following requirements must be met. 

+-----------------------+-------------------------------------------+---------------------------+
| Expression            | Type                                      | Complexity                |
+=======================+===========================================+===========================+
| ``next<i>::type``     | |Bidirectional Iterator|                  | Amortized constant time   |
+-----------------------+-------------------------------------------+---------------------------+
| ``prior<i>::type``    | |Bidirectional Iterator|                  | Amortized constant time   |
+-----------------------+-------------------------------------------+---------------------------+
| ``i::category``       | |Integral Constant|, convertible          | Constant time             |
|                       | to ``bidirectional_iterator_tag``         |                           |
+-----------------------+-------------------------------------------+---------------------------+


Expression semantics
--------------------

.. parsed-literal::

    typedef prior<i>::type j;

:Precondition:
    ``i`` is decrementable 

:Semantics:
    ``j`` is an iterator pointing to the previous element of the 
    sequence

:Postcondition:
    ``j`` is dereferenceable and incrementable 


Invariants
----------

For any bidirectional iterators ``i`` and ``j`` the following invariants 
always hold: 

* If ``i`` is incrementable, then ``prior< next<i>::type >::type`` is a null 
  operation; similarly, if ``i`` is decrementable, ``next< prior<i>::type >::type``
  is a null operation. 


See also
--------

|Iterators|, |Forward Iterator|, |Random Access Iterator|, |Bidirectional Sequence|, |prior|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
