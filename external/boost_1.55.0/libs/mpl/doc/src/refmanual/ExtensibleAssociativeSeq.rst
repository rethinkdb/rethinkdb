.. Sequences/Concepts//Extensible Associative Sequence |80

Extensible Associative Sequence
===============================

Description
-----------

An |Extensible Associative Sequence| is an |Associative Sequence| that supports 
insertion and removal of elements. In contrast to |Extensible Sequence|, 
|Extensible Associative Sequence| does not provide a mechanism for 
inserting an element at a specific position. 


Expression requirements
-----------------------

|In the following table...| ``s`` is an |Associative Sequence|, 
``pos`` is an iterator into ``s``, and ``x`` and ``k`` are arbitrary types.

In addition to the |Associative Sequence| requirements, the following must be met:

+-------------------------------+---------------------------------------+---------------------------+
| Expression                    | Type                                  | Complexity                |
+===============================+=======================================+===========================+
| ``insert<s,x>::type``         | |Extensible Associative Sequence|     | Amortized constant time   |
+-------------------------------+---------------------------------------+---------------------------+
| ``insert<s,pos,x>::type``     | |Extensible Associative Sequence|     | Amortized constant time   |
+-------------------------------+---------------------------------------+---------------------------+
| ``erase_key<s,k>::type``      | |Extensible Associative Sequence|     | Amortized constant time   |
+-------------------------------+---------------------------------------+---------------------------+
| ``erase<s,pos>::type``        | |Extensible Associative Sequence|     | Amortized constant time   |
+-------------------------------+---------------------------------------+---------------------------+
| ``clear<s>::type``            | |Extensible Associative Sequence|     | Amortized constant time   |
+-------------------------------+---------------------------------------+---------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Associative Sequence|.

+-------------------------------+-------------------------------------------------------------------+
| Expression                    | Semantics                                                         |
+===============================+===================================================================+
| ``insert<s,x>::type``         | Inserts ``x`` into ``s``; the resulting sequence ``r`` is         |
|                               | equivalent to ``s`` except that                                   |
|                               | ::                                                                |
|                               |                                                                   |
|                               |     at< r, key_type<s,x>::type >::type                            |
|                               |                                                                   |
|                               | is identical to ``value_type<s,x>::type``; see |insert|.          |
+-------------------------------+-------------------------------------------------------------------+
| ``insert<s,pos,x>::type``     | Equivalent to ``insert<s,x>::type``; ``pos`` is ignored;          |
|                               | see |insert|.                                                     |
+-------------------------------+-------------------------------------------------------------------+
| ``erase_key<s,k>::type``      | Erases elements in ``s`` associated with the key ``k``;           |
|                               | the resulting sequence ``r`` is equivalent to ``s`` except        |
|                               | that ``has_key<r,k>::value == false``; see |erase_key|.           |
+-------------------------------+-------------------------------------------------------------------+
| ``erase<s,pos>::type``        | Erases the element at a specific position; equivalent to          |
|                               | ``erase_key<s, deref<pos>::type >::type``; see |erase|.           |
+-------------------------------+-------------------------------------------------------------------+
| ``clear<s>::type``            | An empty sequence concept-identical to ``s``; see                 |
|                               | |clear|.                                                          |
+-------------------------------+-------------------------------------------------------------------+

.. Invariants
   ----------

   For any extensible associative sequence ``s`` the following invariants always hold: 


Models
------

* |set|
* |map|

.. * |multiset|


See also
--------

|Sequences|, |Associative Sequence|, |insert|, |erase|, |clear|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
