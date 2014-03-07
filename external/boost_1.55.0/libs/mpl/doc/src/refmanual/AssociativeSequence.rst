.. Sequences/Concepts//Associative Sequence |70

Associative Sequence
====================

Description
-----------

An |Associative Sequence| is a |Forward Sequence| that allows efficient retrieval of 
elements based on keys. Unlike associative containers in the C++ Standard Library, 
MPL associative sequences have no associated ordering relation. Instead, 
*type identity* is used to impose an equivalence relation on keys, and the 
order in which sequence elements are traversed during iteration is left 
unspecified.


Definitions
-----------

.. _`key-part`:

.. _`value-part`:

* A *key* is a part of the element type used to identify and retrieve 
  the element within the sequence.

* A *value* is a part of the element type retrievied from the sequence 
  by its key.


Expression requirements
-----------------------

|In the following table...| ``s`` is an |Associative Sequence|, 
``x`` is a sequence element, and ``k`` and ``def`` are arbitrary types.

In addition to the requirements defined in |Forward Sequence|, 
the following must be met:

+-------------------------------+-----------------------------------+---------------------------+
| Expression                    | Type                              | Complexity                |
+===============================+===================================+===========================+
| ``has_key<s,k>::type``        | Boolean |Integral Constant|       | Amortized constant time   |
+-------------------------------+-----------------------------------+---------------------------+
| ``count<s,k>::type``          | |Integral Constant|               | Amortized constant time   |
+-------------------------------+-----------------------------------+---------------------------+
| ``order<s,k>::type``          | |Integral Constant| or ``void_``  | Amortized constant time   |
+-------------------------------+-----------------------------------+---------------------------+
| ``at<s,k>::type``             | Any type                          | Amortized constant time   |
+-------------------------------+-----------------------------------+---------------------------+
| ``at<s,k,def>::type``         | Any type                          | Amortized constant time   |
+-------------------------------+-----------------------------------+---------------------------+
| ``key_type<s,x>::type``       | Any type                          | Amortized constant time   |
+-------------------------------+-----------------------------------+---------------------------+
| ``value_type<s,x>::type``     | Any type                          | Amortized constant time   |
+-------------------------------+-----------------------------------+---------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Forward Sequence|.

+-------------------------------+---------------------------------------------------------------+
| Expression                    | Semantics                                                     |
+===============================+===============================================================+
| ``has_key<s,k>::type``        | |true if and only if| there is one or more                    |
|                               | elements with the key ``k`` in ``s``; see |has_key|.          |
+-------------------------------+---------------------------------------------------------------+
| ``count<s,k>::type``          | The number of elements with the key ``k`` in ``s``;           |
|                               | see |count|.                                                  |
+-------------------------------+---------------------------------------------------------------+
| ``order<s,k>::type``          | A unique unsigned |Integral Constant| associated              |
|                               | with the key ``k`` in the sequence ``s``; see |order|.        |
+-------------------------------+---------------------------------------------------------------+
| .. parsed-literal::           | The first element associated with the key ``k``               |
|                               | in the sequence ``s``; see |at|.                              |
|    at<s,k>::type              |                                                               |
|    at<s,k,def>::type          |                                                               |
+-------------------------------+---------------------------------------------------------------+
| ``key_type<s,x>::type``       | The key part of the element ``x`` that would be               |
|                               | used to identify ``x`` in ``s``; see |key_type|.              |
+-------------------------------+---------------------------------------------------------------+
| ``value_type<s,x>::type``     | The value part of the element ``x`` that would be             |
|                               | used for ``x`` in ``s``; see |value_type|.                    |
+-------------------------------+---------------------------------------------------------------+


.. Invariants
   ----------

   For any associative sequence ``s`` the following invariants always hold: 

    * ???


Models
------

* |set|
* |map|

.. * |multiset|


See also
--------

|Sequences|, |Extensible Associative Sequence|, |has_key|, |count|, |order|, |at|, |key_type|, |value_type|


.. |key| replace:: `key`_
.. _`key`: `key-part`_

.. |value| replace:: `value`_
.. _`value`: `value-part`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
