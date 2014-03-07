.. Sequences/Concepts//Bidirectional Sequence |20

Bidirectional Sequence
======================

Description
-----------

A |Bidirectional Sequence| is a |Forward Sequence| whose iterators model
|Bidirectional Iterator|. 

Refinement of
-------------

|Forward Sequence|


Expression requirements
-----------------------

In addition to the requirements defined in |Forward Sequence|, 
for any |Bidirectional Sequence| ``s`` the following must be met:

+---------------------------+-----------------------------------+---------------------------+
| Expression                | Type                              | Complexity                |
+===========================+===================================+===========================+
| ``begin<s>::type``        | |Bidirectional Iterator|          | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+
| ``end<s>::type``          | |Bidirectional Iterator|          | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+
| ``back<s>::type``         | Any type                          | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Forward Sequence|.

+---------------------------+-----------------------------------------------------------------------+
| Expression                | Semantics                                                             |
+===========================+=======================================================================+
| ``back<s>::type``         | The last element in the sequence; see |back|.                         |
+---------------------------+-----------------------------------------------------------------------+


Models
------

* |vector|
* |range_c|


See also
--------

|Sequences|, |Forward Sequence|, |Random Access Sequence|, |Bidirectional Iterator|, |begin| / |end|, |back|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
