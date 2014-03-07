.. Sequences/Classes//list |20

list
====

Description
-----------

A ``list`` is a |variadic|, `forward`__, `extensible`__ sequence of types that 
supports constant-time insertion and removal of elements  at the beginning, and 
linear-time insertion and removal of elements at the end and in the middle. 

__ `Forward Sequence`_
__ `Extensible Sequence`_

Header
------

+-------------------+-------------------------------------------------------+
| Sequence form     | Header                                                |
+===================+=======================================================+
| Variadic          | ``#include <boost/mpl/list.hpp>``                     |
+-------------------+-------------------------------------------------------+
| Numbered          | ``#include <boost/mpl/list/list``\ *n*\ ``.hpp>``     |
+-------------------+-------------------------------------------------------+


Model of
--------

* |Variadic Sequence|
* |Forward Sequence|
* |Extensible Sequence|
* |Front Extensible Sequence|


Expression semantics
--------------------

In the following table, ``l`` is a ``list``, ``pos`` and ``last`` are iterators into ``l``, 
``r`` is a |Forward Sequence|, and |t1...tn| and ``x`` are arbitrary types.

+---------------------------------------+-----------------------------------------------------------+
| Expression                            | Semantics                                                 |
+=======================================+===========================================================+
| .. parsed-literal::                   | ``list`` of elements |t1...tn|; see                       |
|                                       | |Variadic Sequence|.                                      |
|    list<|t1...tn|>                    |                                                           |
|    list\ *n*\ <|t1...tn|>             |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| .. parsed-literal::                   | Identical to ``list``\ *n*\ ``<``\ |t1...tn|\ ``>``;      |
|                                       | see |Variadic Sequence|.                                  |
|    list<|t1...tn|>::type              |                                                           |
|    list\ *n*\ <|t1...tn|>::type       |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``begin<l>::type``                    | An iterator to the beginning of ``l``;                    |
|                                       | see |Forward Sequence|.                                   |
+---------------------------------------+-----------------------------------------------------------+
| ``end<l>::type``                      | An iterator to the end of ``l``;                          |
|                                       | see |Forward Sequence|.                                   |
+---------------------------------------+-----------------------------------------------------------+
| ``size<l>::type``                     | The size of ``l``; see |Forward Sequence|.                |
+---------------------------------------+-----------------------------------------------------------+
| ``empty<l>::type``                    | |true if and only if| ``l`` is empty; see                 |
|                                       | |Forward Sequence|.                                       |
+---------------------------------------+-----------------------------------------------------------+
| ``front<l>::type``                    | The first element in ``l``; see                           |
|                                       | |Forward Sequence|.                                       |
+---------------------------------------+-----------------------------------------------------------+
| ``insert<l,pos,x>::type``             | A new ``list`` of following elements:                     |
|                                       | [``begin<l>::type``, ``pos``), ``x``,                     |
|                                       | [``pos``, ``end<l>::type``); see |Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``insert_range<l,pos,r>::type``       | A new ``list`` of following elements:                     |
|                                       | [``begin<l>::type``, ``pos``),                            |
|                                       | [``begin<r>::type``, ``end<r>::type``)                    |
|                                       | [``pos``, ``end<l>::type``); see |Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<l,pos>::type``                | A new ``list`` of following elements:                     |
|                                       | [``begin<l>::type``, ``pos``),                            |
|                                       | [``next<pos>::type``, ``end<l>::type``); see              |
|                                       | |Extensible Sequence|.                                    |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<l,pos,last>::type``           | A new ``list`` of following elements:                     |
|                                       | [``begin<l>::type``, ``pos``),                            |
|                                       | [``last``, ``end<l>::type``); see |Extensible Sequence|.  |
+---------------------------------------+-----------------------------------------------------------+
| ``clear<l>::type``                    | An empty ``list``; see |Extensible Sequence|.             |
+---------------------------------------+-----------------------------------------------------------+
| ``push_front<l,x>::type``             | A new ``list`` containing ``x`` as its first              |
|                                       | element; see |Front Extensible Sequence|.                 |
+---------------------------------------+-----------------------------------------------------------+
| ``pop_front<l>::type``                | A new ``list`` containing all but the first elements      |
|                                       | of ``l`` in  the same order; see                          |
|                                       | |Front Extensible Sequence|.                              |
+---------------------------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::
    
    typedef list<float,double,long double> floats;
    typedef push_front<floating_types,int>::type types;
    
    BOOST_MPL_ASSERT(( is_same< front<types>::type, int > ));


See also
--------

|Sequences|, |Variadic Sequence|, |Forward Sequence|, |Extensible Sequence|, |vector|, |list_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
