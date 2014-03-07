.. Iterators/Concepts//Forward Iterator |10

Forward Iterator
================

Description
-----------

A |Forward Iterator| ``i`` is a type that represents a positional reference
to an element of a |Forward Sequence|. It allows to access the element through 
a dereference operation, and provides a way to obtain an iterator to 
the next element in a sequence. 

.. A [Forward Iterator] guarantees a linear traversal over 
   the sequence.


Definitions
-----------

* An iterator can be `dereferenceable`, meaning that ``deref<i>::type``
  is a well-defined expression. 

* An iterator is `past-the-end` if it points beyond the last element of a 
  sequence; past-the-end iterators are non-dereferenceable.

* An iterator ``i`` is `incrementable` if there is a "next" iterator, that 
  is, if ``next<i>::type`` expression is well-defined; past-the-end iterators are 
  not incrementable.

* Two iterators into the same sequence are `equivalent` if they have the same
  type.

* An iterator ``j`` is `reachable` from an iterator ``i`` if , after recursive 
  application of ``next`` metafunction to ``i`` a finite number of times, ``i`` 
  is equivalent to ``j``.

* The notation [``i``,\ ``j``) refers to a `range` of iterators beginning with 
  ``i`` and up to but not including ``j``.

* The range [``i``,\ ``j``) is a `valid range` if ``j`` is reachable from ``i``. 


Expression requirements
-----------------------

+-----------------------+-------------------------------------------+---------------------------+
| Expression            | Type                                      | Complexity                |
+=======================+===========================================+===========================+
| ``deref<i>::type``    | Any type                                  | Amortized constant time   |
+-----------------------+-------------------------------------------+---------------------------+
| ``next<i>::type``     | |Forward Iterator|                        | Amortized constant time   |
+-----------------------+-------------------------------------------+---------------------------+
| ``i::category``       | |Integral Constant|, convertible          | Constant time             |
|                       | to ``forward_iterator_tag``               |                           |
+-----------------------+-------------------------------------------+---------------------------+


Expression semantics
--------------------


.. parsed-literal::

    typedef deref<i>::type j;

:Precondition:
    ``i`` is dereferenceable 

:Semantics:
    ``j`` is identical to the type of the pointed element  


.. ..........................................................................

.. parsed-literal::

    typedef next<i>::type j;

:Precondition:
    ``i`` is incrementable 

:Semantics:
    ``j`` is the next iterator in a sequence 

:Postcondition:
    ``j`` is dereferenceable or past-the-end  


.. ..........................................................................

.. parsed-literal::

    typedef i::category c;

:Semantics:
    ``c`` is identical to the iterator's category tag


Invariants
----------

For any forward iterators ``i`` and ``j`` the following invariants always hold: 

* ``i`` and ``j`` are equivalent if and only if they are pointing to the same 
  element.

* If ``i`` is dereferenceable, and ``j`` is equivalent to ``i``, then ``j`` is 
  dereferenceable as well.

* If ``i`` and ``j`` are equivalent and dereferenceable, then ``deref<i>::type`` 
  and ``deref<j>::type`` are identical. 

* If ``i`` is incrementable, and ``j`` is equivalent to ``i``, then ``j`` is 
  incrementable as well.

* If ``i`` and ``j`` are equivalent and incrementable, then ``next<i>::type`` 
  and ``next<j>::type`` are equivalent. 


See also
--------

|Iterators|, |Bidirectional Iterator|, |Forward Sequence|, |deref|, |next|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
