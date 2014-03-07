.. Sequences/Concepts//Front Extensible Sequence |50

Front Extensible Sequence
=========================

Description
-----------

A |Front Extensible Sequence| is an |Extensible Sequence| that supports amortized constant 
time insertion and removal operations at the beginning. 

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
| ``push_front<s,x>::type``     | |Front Extensible Sequence|   | Amortized constant time   |
+-------------------------------+-------------------------------+---------------------------+
| ``pop_front<s>::type``        | |Front Extensible Sequence|   | Amortized constant time   |
+-------------------------------+-------------------------------+---------------------------+
| ``front<s>::type``            | Any type                      | Amortized constant time   |
+-------------------------------+-------------------------------+---------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Extensible Sequence|.

+-------------------------------+-----------------------------------------------------------+
| Expression                    | Semantics                                                 |
+===============================+===========================================================+
| ``push_front<s,x>::type``     | Equivalent to ``insert<s,begin<s>::type,x>::type``;       |
|                               | see |push_front|.                                         |
+-------------------------------+-----------------------------------------------------------+
| ``pop_front<v>::type``        | Equivalent to ``erase<s,begin<s>::type>::type``;          |
|                               | see |pop_front|.                                          |
+-------------------------------+-----------------------------------------------------------+
| ``front<s>::type``            | The first element in the sequence; see |front|.           |
+-------------------------------+-----------------------------------------------------------+


Models
------

* |vector|
* |list|


See also
--------

|Sequences|, |Extensible Sequence|, |Back Extensible Sequence|, |push_front|, |pop_front|, |front|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
