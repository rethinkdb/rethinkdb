.. Sequences/Concepts//Extensible Sequence |40

Extensible Sequence
===================

Description
-----------

An |Extensible Sequence| is a sequence that supports insertion and removal of 
elements. Extensibility is orthogonal to sequence traversal characteristics.


Expression requirements
-----------------------

For any |Extensible Sequence| ``s``, its iterators ``pos`` and ``last``, 
|Forward Sequence| ``r``, and any type ``x``, the following expressions must
be valid:

+-----------------------------------+---------------------------+---------------------------+
| Expression                        | Type                      | Complexity                |
+===================================+===========================+===========================+
| ``insert<s,pos,x>::type``         | |Extensible Sequence|     | Unspecified               |
+-----------------------------------+---------------------------+---------------------------+
| ``insert_range<s,pos,r>::type``   | |Extensible Sequence|     | Unspecified               |
+-----------------------------------+---------------------------+---------------------------+
| ``erase<s,pos>::type``            | |Extensible Sequence|     | Unspecified               |
+-----------------------------------+---------------------------+---------------------------+
| ``erase<s,pos,last>::type``       | |Extensible Sequence|     | Unspecified               |
+-----------------------------------+---------------------------+---------------------------+
| ``clear<s>::type``                | |Extensible Sequence|     | Constant time             |
+-----------------------------------+---------------------------+---------------------------+

Expression semantics
--------------------

+-----------------------------------+---------------------------------------------------------------+
| Expression                        | Semantics                                                     |
+===================================+===============================================================+
| ``insert<s,pos,x>::type``         | A new sequence, concept-identical to ``s``, of                |
|                                   | the following elements:                                       |
|                                   | [``begin<s>::type``, ``pos``), ``x``,                         |
|                                   | [``pos``, ``end<s>::type``); see |insert|.                    |
+-----------------------------------+---------------------------------------------------------------+
| ``insert_range<s,pos,r>::type``   | A new sequence, concept-identical to ``s``, of                |
|                                   | the following elements:                                       |
|                                   | [``begin<s>::type``, ``pos``),                                |
|                                   | [``begin<r>::type``, ``end<r>::type``),                       |
|                                   | [``pos``, ``end<s>::type``); see |insert_range|.              |
+-----------------------------------+---------------------------------------------------------------+
| ``erase<s,pos>::type``            | A new sequence, concept-identical to ``s``, of                |
|                                   | the following elements:                                       |
|                                   | [``begin<s>::type``, ``pos``),                                |
|                                   | [``next<pos>::type``, ``end<s>::type``); see |erase|.         |
+-----------------------------------+---------------------------------------------------------------+
| ``erase<s,pos,last>::type``       | A new sequence, concept-identical to ``s``, of                |
|                                   | the following elements:                                       |
|                                   | [``begin<s>::type``, ``pos``),                                |
|                                   | [``last``, ``end<s>::type``); see |erase|.                    |
+-----------------------------------+---------------------------------------------------------------+
| ``clear<s>::type``                | An empty sequence concept-identical to ``s``; see             |
|                                   | |clear|.                                                      |
+-----------------------------------+---------------------------------------------------------------+


Models
------

* |vector|
* |list|


See also
--------

|Sequences|, |Back Extensible Sequence|, |insert|, |insert_range|, |erase|, |clear|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
