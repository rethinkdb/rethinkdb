.. Sequences/Concepts//Random Access Sequence |30

Random Access Sequence
======================

Description
-----------

A |Random Access Sequence| is a |Bidirectional Sequence| whose iterators model
|Random Access Iterator|. A random access sequence guarantees amortized constant 
time access to an arbitrary sequence element. 

Refinement of
-------------

|Bidirectional Sequence|


Expression requirements
-----------------------

In addition to the requirements defined in |Bidirectional Sequence|, 
for any |Random Access Sequence| ``s`` the following must be met:

+---------------------------+-----------------------------------+---------------------------+
| Expression                | Type                              | Complexity                |
+===========================+===================================+===========================+
| ``begin<s>::type``        | |Random Access Iterator|          | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+
| ``end<s>::type``          | |Random Access Iterator|          | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+
| ``at<s,n>::type``         | Any type                          | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+


Expression semantics
--------------------

Semantics of an expression is defined only where it differs from, or is not 
defined in |Bidirectional Sequence|.

+---------------------------+-----------------------------------------------------------------------+
| Expression                | Semantics                                                             |
+===========================+=======================================================================+
| ``at<s,n>::type``         | The ``n``\ th element from the beginning of the sequence; see |at|.   |
+---------------------------+-----------------------------------------------------------------------+


Models
------

* |vector|
* |range_c|


See also
--------

|Sequences|, |Bidirectional Sequence|, |Extensible Sequence|, |Random Access Iterator|, 
|begin| / |end|, |at|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
