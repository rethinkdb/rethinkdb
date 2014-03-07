.. Sequences/Concepts//Back Extensible Sequence |60

Back Extensible Sequence
========================

Description
-----------

A |Back Extensible Sequence| is an |Extensible Sequence| that supports amortized constant 
time insertion and removal operations at the end.

Refinement of
-------------

|Extensible Sequence|


Expression requirements
-----------------------

In addition to the requirements defined in |Extensible Sequence|, 
for any |Back Extensible Sequence| ``s`` the following must be met:

+-------------------------------+-------------------------------+---------------------------+
| Expression                    | Type                          | Complexity                |
+===============================+===============================+===========================+
| ``push_back<s,x>::type``      | |Back Extensible Sequence|    | Amortized constant time   |
+-------------------------------+-------------------------------+---------------------------+
| ``pop_back<s>::type``         | |Back Extensible Sequence|    | Amortized constant time   |
+-------------------------------+-------------------------------+---------------------------+
| ``back<s>::type``             | Any type                      | Amortized constant time   |
+-------------------------------+-------------------------------+---------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Extensible Sequence|.

+-------------------------------+-----------------------------------------------------------+
| Expression                    | Semantics                                                 |
+===============================+===========================================================+
| ``push_back<s,x>::type``      | Equivalent to ``insert<s,end<s>::type,x>::type``;         |
|                               | see |push_back|.                                          |
+-------------------------------+-----------------------------------------------------------+
| ``pop_back<v>::type``         | Equivalent to ``erase<s,end<s>::type>::type``;            |
|                               | see |pop_back|.                                           |
+-------------------------------+-----------------------------------------------------------+
| ``back<s>::type``             | The last element in the sequence; see |back|.             |
+-------------------------------+-----------------------------------------------------------+


Models
------

* |vector|
* |deque|


See also
--------

|Sequences|, |Extensible Sequence|, |Front Extensible Sequence|, |push_back|, |pop_back|, |back|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
