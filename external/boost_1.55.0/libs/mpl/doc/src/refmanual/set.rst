.. Sequences/Classes//set |40

set
===

Description
-----------

``set`` is a |variadic|, `associative`__, `extensible`__ sequence of types that 
supports constant-time insertion and removal of elements, and testing for membership.
A ``set`` may contain at most one element for each key.

__ `Associative Sequence`_
__ `Extensible Associative Sequence`_

Header
------

+-------------------+-------------------------------------------------------+
| Sequence form     | Header                                                |
+===================+=======================================================+
| Variadic          | ``#include <boost/mpl/set.hpp>``                      |
+-------------------+-------------------------------------------------------+
| Numbered          | ``#include <boost/mpl/set/set``\ *n*\ ``.hpp>``       |
+-------------------+-------------------------------------------------------+


Model of
--------

* |Variadic Sequence|
* |Associative Sequence|
* |Extensible Associative Sequence|


Expression semantics
--------------------

In the following table, ``s`` is an instance of ``set``, ``pos`` is an iterator into ``s``, 
and ``x``, ``k``, and |t1...tn| is a set of *unique* arbitrary types. [*Note:* See `below`__ for
an example of how to construct a ``set`` from a list of potentially non-unique types |--| *end note*\]

__ nonunique-set-example_

+---------------------------------------+-----------------------------------------------------------+
| Expression                            | Semantics                                                 |
+=======================================+===========================================================+
| .. parsed-literal::                   | ``set`` of elements |t1...tn|; see                        |
|                                       | |Variadic Sequence|.                                      |
|    set<|t1...tn|>                     |                                                           |
|    set\ *n*\ <|t1...tn|>              |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| .. parsed-literal::                   | Identical to ``set``\ *n*\ ``<``\ |t1...tn|\ ``>``;       |
|                                       | see |Variadic Sequence|.                                  |
|    set<|t1...tn|>::type               |                                                           |
|    set\ *n*\ <|t1...tn|>::type        |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``begin<s>::type``                    | An iterator pointing to the beginning of ``s``;           |
|                                       | see |Associative Sequence|.                               |
+---------------------------------------+-----------------------------------------------------------+
| ``end<s>::type``                      | An iterator pointing to the end of ``s``;                 |
|                                       | see |Associative Sequence|.                               |
+---------------------------------------+-----------------------------------------------------------+
| ``size<s>::type``                     | The size of ``s``; see |Associative Sequence|.            |
+---------------------------------------+-----------------------------------------------------------+
| ``empty<s>::type``                    | |true if and only if| ``s`` is empty; see                 |
|                                       | |Associative Sequence|.                                   |
+---------------------------------------+-----------------------------------------------------------+
| ``front<s>::type``                    | The first element in ``s``; see                           |
|                                       | |Associative Sequence|.                                   |
+---------------------------------------+-----------------------------------------------------------+
| ``has_key<s,k>::type``                | |true if and only if| there is one or more elements       |
|                                       | with the key ``k`` in ``s``; see |Associative Sequence|.  |
+---------------------------------------+-----------------------------------------------------------+
| ``count<s,k>::type``                  | The number of elements with the key ``k`` in ``s``;       |
|                                       | see |Associative Sequence|.                               |
+---------------------------------------+-----------------------------------------------------------+
| ``order<s,k>::type``                  | A unique unsigned |Integral Constant| associated with     |
|                                       | the key ``k`` in ``s``; see |Associative Sequence|.       |
+---------------------------------------+-----------------------------------------------------------+
| .. parsed-literal::                   | The element associated with the key ``k`` in              |
|                                       | ``s``; see |Associative Sequence|.                        |
|    at<s,k>::type                      |                                                           |
|    at<s,k,def>::type                  |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``key_type<s,x>::type``               | Identical to ``x``; see |Associative Sequence|.           |
+---------------------------------------+-----------------------------------------------------------+
| ``value_type<s,x>::type``             | Identical to ``x``; see |Associative Sequence|.           |
+---------------------------------------+-----------------------------------------------------------+
| ``insert<s,x>::type``                 | A new ``set`` equivalent to ``s`` except that             |
|                                       | ::                                                        |
|                                       |                                                           |
|                                       |     at< t, key_type<s,x>::type >::type                    |
|                                       |                                                           |
|                                       | is identical to ``value_type<s,x>::type``.                |
+---------------------------------------+-----------------------------------------------------------+
| ``insert<s,pos,x>::type``             | Equivalent to ``insert<s,x>::type``; ``pos`` is ignored.  |
+---------------------------------------+-----------------------------------------------------------+
| ``erase_key<s,k>::type``              | A new ``set`` equivalent to ``s`` except that             |
|                                       | ``has_key<t, k>::value == false``.                        |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<s,pos>::type``                | Equivalent to ``erase<s, deref<pos>::type >::type``.      |
+---------------------------------------+-----------------------------------------------------------+
| ``clear<s>::type``                    | An empty ``set``; see |clear|.                            |
+---------------------------------------+-----------------------------------------------------------+


Example
-------

Basic ``set`` invariants:

.. parsed-literal::

    typedef set< int,long,double,int_<5> > s;

    BOOST_MPL_ASSERT_RELATION( size<s>::value, ==, 4 );
    BOOST_MPL_ASSERT_NOT(( empty<s> ));
    
    BOOST_MPL_ASSERT(( is_same< at<s,int>::type, int > ));
    BOOST_MPL_ASSERT(( is_same< at<s,long>::type, long > ));
    BOOST_MPL_ASSERT(( is_same< at<s,int_<5> >::type, int_<5> > ));
    BOOST_MPL_ASSERT(( is_same< at<s,char>::type, void\_ > ));


.. _nonunique-set-example:

Constructing a ``set`` from a list of potentially non-unique types:

.. parsed-literal::

    typedef fold<
          vector<int,int,long,long>
        , set0<>
        , insert<_1,_2>
        >::type s;

    BOOST_MPL_ASSERT_RELATION( size<s>::value, ==, 2 );


See also
--------

|Sequences|, |Variadic Sequence|, |Associative Sequence|, |Extensible Associative Sequence|, |set_c|, |map|, |vector|


.. copyright:: Copyright ©  2001-2011 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
