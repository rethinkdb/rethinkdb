.. Sequences/Classes//map |50

map
===

Description
-----------

``map`` is a |variadic|, `associative`__, `extensible`__ sequence of type pairs that 
supports constant-time insertion and removal of elements, and testing for membership.
A ``map`` may contain at most one element for each key.

__ `Associative Sequence`_
__ `Extensible Associative Sequence`_

Header
------

+-------------------+-------------------------------------------------------+
| Sequence form     | Header                                                |
+===================+=======================================================+
| Variadic          | ``#include <boost/mpl/map.hpp>``                      |
+-------------------+-------------------------------------------------------+
| Numbered          | ``#include <boost/mpl/map/map``\ *n*\ ``.hpp>``       |
+-------------------+-------------------------------------------------------+


Model of
--------

* |Variadic Sequence|
* |Associative Sequence|
* |Extensible Associative Sequence|


Expression semantics
--------------------

|In the following table...| ``m`` is an instance of ``map``,
``pos`` is an iterator into ``m``, ``x`` and |p1...pn| are ``pair``\ s, and ``k`` is an arbitrary type.

+---------------------------------------+-----------------------------------------------------------+
| Expression                            | Semantics                                                 |
+=======================================+===========================================================+
| .. parsed-literal::                   | ``map`` of elements |p1...pn|; see                        |
|                                       | |Variadic Sequence|.                                      |
|    map<|p1...pn|>                     |                                                           |
|    map\ *n*\ <|p1...pn|>              |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| .. parsed-literal::                   | Identical to ``map``\ *n*\ ``<``\ |p1...pn|\ ``>``;       |
|                                       | see |Variadic Sequence|.                                  |
|    map<|p1...pn|>::type               |                                                           |
|    map\ *n*\ <|p1...pn|>::type        |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``begin<m>::type``                    | An iterator pointing to the beginning of ``m``;           |
|                                       | see |Associative Sequence|.                               |
+---------------------------------------+-----------------------------------------------------------+
| ``end<m>::type``                      | An iterator pointing to the end of ``m``;                 |
|                                       | see |Associative Sequence|.                               |
+---------------------------------------+-----------------------------------------------------------+
| ``size<m>::type``                     | The size of ``m``; see |Associative Sequence|.            |
+---------------------------------------+-----------------------------------------------------------+
| ``empty<m>::type``                    | |true if and only if| ``m`` is empty; see                 |
|                                       | |Associative Sequence|.                                   |
+---------------------------------------+-----------------------------------------------------------+
| ``front<m>::type``                    | The first element in ``m``; see                           |
|                                       | |Associative Sequence|.                                   |
+---------------------------------------+-----------------------------------------------------------+
| ``has_key<m,k>::type``                | Queries the presence of elements with the key ``k`` in    |
|                                       | ``m``; see |Associative Sequence|.                        |
+---------------------------------------+-----------------------------------------------------------+
| ``count<m,k>::type``                  | The number of elements with the key ``k`` in ``m``;       |
|                                       | see |Associative Sequence|.                               |
+---------------------------------------+-----------------------------------------------------------+
| ``order<m,k>::type``                  | A unique unsigned |Integral Constant| associated with     |
|                                       | the key ``k`` in ``m``; see |Associative Sequence|.       |
+---------------------------------------+-----------------------------------------------------------+
| .. parsed-literal::                   | The element associated with the key ``k`` in              |
|                                       | ``m``; see |Associative Sequence|.                        |
|    at<m,k>::type                      |                                                           |
|    at<m,k,default>::type              |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``key_type<m,x>::type``               | Identical to ``x::first``; see |Associative Sequence|.    |
+---------------------------------------+-----------------------------------------------------------+
| ``value_type<m,x>::type``             | Identical to ``x::second``; see |Associative Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``insert<m,x>::type``                 | A new ``map``, ``t``, equivalent to ``m`` except that     |
|                                       | ::                                                        |
|                                       |                                                           |
|                                       |     at< t, key_type<m,x>::type >::type                    |
|                                       |                                                           |
|                                       | is identical to ``value_type<m,x>::type``.                |
+---------------------------------------+-----------------------------------------------------------+
| ``insert<m,pos,x>::type``             | Equivalent to ``insert<m,x>::type``; ``pos`` is ignored.  |
+---------------------------------------+-----------------------------------------------------------+
| ``erase_key<m,k>::type``              | A new ``map``, ``t``, equivalent to ``m`` except that     |
|                                       | ``has_key<t, k>::value == false``.                        |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<m,pos>::type``                | Equivalent to ``erase<m, deref<pos>::type >::type``.      |
+---------------------------------------+-----------------------------------------------------------+
| ``clear<m>::type``                    | An empty ``map``; see |clear|.                            |
+---------------------------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::

    typedef map<
          pair<int,unsigned>
        , pair<char,unsigned char>
        , pair<long_<5>,char[17]>
        , pair<int[42],bool>
        > m;

    BOOST_MPL_ASSERT_RELATION( size<m>::value, ==, 4 );
    BOOST_MPL_ASSERT_NOT(( empty<m> ));
    
    BOOST_MPL_ASSERT(( is_same< at<m,int>::type, unsigned > ));
    BOOST_MPL_ASSERT(( is_same< at<m,long_<5> >::type, char[17] > ));
    BOOST_MPL_ASSERT(( is_same< at<m,int[42]>::type, bool > ));
    BOOST_MPL_ASSERT(( is_same< at<m,long>::type, void\_ > ));


See also
--------

|Sequences|, |Variadic Sequence|, |Associative Sequence|, |Extensible Associative Sequence|, |set|, |vector|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
