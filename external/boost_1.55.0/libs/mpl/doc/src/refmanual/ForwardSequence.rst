.. Sequences/Concepts//Forward Sequence |10

Forward Sequence
================

Description
-----------

A |Forward Sequence| is an MPL concept representing a compile-time sequence of 
elements. Sequence elements are 
types, and are accessible through |iterators|. The |begin| and |end| metafunctions
provide iterators delimiting the range of the sequence 
elements.  A sequence guarantees that its elements are arranged in a definite, 
but possibly unspecified, order. Every MPL sequence is a |Forward Sequence|.

Definitions
-----------

* The *size* of a sequence is the number of elements it contains. The size is a 
  nonnegative number.

* A sequence is *empty* if its size is zero.


Expression requirements
-----------------------

For any |Forward Sequence| ``s`` the following expressions must be valid:

+---------------------------+-----------------------------------+---------------------------+
| Expression                | Type                              | Complexity                |
+===========================+===================================+===========================+
| ``begin<s>::type``        | |Forward Iterator|                | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+
| ``end<s>::type``          | |Forward Iterator|                | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+
| ``size<s>::type``         | |Integral Constant|               | Unspecified               |
+---------------------------+-----------------------------------+---------------------------+
| ``empty<s>::type``        | Boolean |Integral Constant|       | Constant time             |
+---------------------------+-----------------------------------+---------------------------+
| ``front<s>::type``        | Any type                          | Amortized constant time   |
+---------------------------+-----------------------------------+---------------------------+


Expression semantics
--------------------

+---------------------------+-----------------------------------------------------------------------+
| Expression                | Semantics                                                             |
+===========================+=======================================================================+
| ``begin<s>::type``        | An iterator to the first element of the sequence; see |begin|.        |
+---------------------------+-----------------------------------------------------------------------+
| ``end<s>::type``          | A past-the-end iterator to the sequence; see |end|.                   |
+---------------------------+-----------------------------------------------------------------------+
| ``size<s>::type``         | The size of the sequence; see |size|.                                 |
+---------------------------+-----------------------------------------------------------------------+
| ``empty<s>::type``        | |true if and only if| the sequence is empty; see |empty|.             |
+---------------------------+-----------------------------------------------------------------------+
| ``front<s>::type``        | The first element in the sequence; see |front|.                       |
+---------------------------+-----------------------------------------------------------------------+


Invariants
----------

For any |Forward Sequence| ``s`` the following invariants always hold: 

* [``begin<s>::type``, ``end<s>::type``) is always a valid range.

* An algorithm that iterates through the range [``begin<s>::type``, ``end<s>::type``) 
  will pass through every element of ``s`` exactly once.

* ``begin<s>::type`` is identical to ``end<s>::type`` if and only if ``s`` is empty.

* Two different iterations through ``s`` will access its elements in the same order. 


Models
------

* |vector|
* |map|
* |range_c|
* |iterator_range|
* |filter_view|

See also
--------

|Sequences|, |Bidirectional Sequence|, |Forward Iterator|, |begin| / |end|, |size|, |empty|, |front|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
