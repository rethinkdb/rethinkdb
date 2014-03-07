.. Sequences/Classes//vector |10

vector
======

Description
-----------

``vector`` is a |variadic|, `random access`__, `extensible`__ sequence of types that 
supports constant-time insertion and removal of elements at both ends, and 
linear-time insertion and removal of elements in the middle. On compilers that 
support the ``typeof`` extension, ``vector`` is the simplest and in many cases the
most efficient sequence.

__ `Random Access Sequence`_
__ `Extensible Sequence`_

Header
------

+-------------------+-------------------------------------------------------+
| Sequence form     | Header                                                |
+===================+=======================================================+
| Variadic          | ``#include <boost/mpl/vector.hpp>``                   |
+-------------------+-------------------------------------------------------+
| Numbered          | ``#include <boost/mpl/vector/vector``\ *n*\ ``.hpp>`` |
+-------------------+-------------------------------------------------------+

Model of
--------

* |Variadic Sequence|
* |Random Access Sequence|
* |Extensible Sequence|
* |Back Extensible Sequence|
* |Front Extensible Sequence|


Expression semantics
--------------------

In the following table, ``v`` is an instance of ``vector``, ``pos`` and ``last`` are iterators 
into ``v``, ``r`` is a |Forward Sequence|, ``n`` is an |Integral Constant|, and ``x`` and 
|t1...tn| are arbitrary types.

+---------------------------------------+-----------------------------------------------------------+
| Expression                            | Semantics                                                 |
+=======================================+===========================================================+
| .. parsed-literal::                   | ``vector`` of elements |t1...tn|; see                     |
|                                       | |Variadic Sequence|.                                      |
|    vector<|t1...tn|>                  |                                                           |
|    vector\ *n*\ <|t1...tn|>           |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| .. parsed-literal::                   | Identical to ``vector``\ *n*\ ``<``\ |t1...tn|\ ``>``;    |
|                                       | see |Variadic Sequence|.                                  |
|    vector<|t1...tn|>::type            |                                                           |
|    vector\ *n*\ <|t1...tn|>::type     |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``begin<v>::type``                    | An iterator pointing to the beginning of ``v``;           |
|                                       | see |Random Access Sequence|.                             |
+---------------------------------------+-----------------------------------------------------------+
| ``end<v>::type``                      | An iterator pointing to the end of ``v``;                 |
|                                       | see |Random Access Sequence|.                             |
+---------------------------------------+-----------------------------------------------------------+
| ``size<v>::type``                     | The size of ``v``; see |Random Access Sequence|.          |
+---------------------------------------+-----------------------------------------------------------+
| ``empty<v>::type``                    | |true if and only if| the sequence is empty;              |
|                                       | see |Random Access Sequence|.                             |
+---------------------------------------+-----------------------------------------------------------+
| ``front<v>::type``                    | The first element in ``v``; see                           |
|                                       | |Random Access Sequence|.                                 |
+---------------------------------------+-----------------------------------------------------------+
| ``back<v>::type``                     | The last element in ``v``; see                            |
|                                       | |Random Access Sequence|.                                 |
+---------------------------------------+-----------------------------------------------------------+
| ``at<v,n>::type``                     | The ``n``\ th element from the beginning of ``v``; see    |
|                                       | |Random Access Sequence|.                                 |
+---------------------------------------+-----------------------------------------------------------+
| ``insert<v,pos,x>::type``             | A new ``vector`` of following elements:                   |
|                                       | [``begin<v>::type``, ``pos``), ``x``,                     |
|                                       | [``pos``, ``end<v>::type``); see |Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``insert_range<v,pos,r>::type``       | A new ``vector`` of following elements:                   |
|                                       | [``begin<v>::type``, ``pos``),                            |
|                                       | [``begin<r>::type``, ``end<r>::type``)                    |
|                                       | [``pos``, ``end<v>::type``); see |Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<v,pos>::type``                | A new ``vector`` of following elements:                   |
|                                       | [``begin<v>::type``, ``pos``),                            |
|                                       | [``next<pos>::type``, ``end<v>::type``); see              |
|                                       | |Extensible Sequence|.                                    |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<v,pos,last>::type``           | A new ``vector`` of following elements:                   |
|                                       | [``begin<v>::type``, ``pos``),                            |
|                                       | [``last``, ``end<v>::type``); see |Extensible Sequence|.  |
+---------------------------------------+-----------------------------------------------------------+
| ``clear<v>::type``                    | An empty ``vector``; see |Extensible Sequence|.           |
+---------------------------------------+-----------------------------------------------------------+
| ``push_back<v,x>::type``              | A new ``vector`` of following elements:                   | 
|                                       | |begin/end<v>|, ``x``;                                    |
|                                       | see |Back Extensible Sequence|.                           |
+---------------------------------------+-----------------------------------------------------------+
| ``pop_back<v>::type``                 | A new ``vector`` of following elements:                   |
|                                       | [``begin<v>::type``, ``prior< end<v>::type >::type``);    |
|                                       | see |Back Extensible Sequence|.                           |
+---------------------------------------+-----------------------------------------------------------+
| ``push_front<v,x>::type``             | A new ``vector`` of following elements:                   |
|                                       | ``x``, |begin/end<v>|; see |Front Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``pop_front<v>::type``                | A new ``vector`` of following elements:                   |
|                                       | [``next< begin<v>::type >::type``, ``end<v>::type``);     |
|                                       | see |Front Extensible Sequence|.                          |
+---------------------------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::
    
    typedef vector<float,double,long double> floats;
    typedef push_back<floats,int>::type types;

    BOOST_MPL_ASSERT(( |is_same|\< at_c<types,3>::type, int > ));


See also
--------

|Sequences|, |Variadic Sequence|, |Random Access Sequence|, |Extensible Sequence|, |vector_c|, |list|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
