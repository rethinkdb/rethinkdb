.. Sequences/Classes//string |100

string
======

Description
-----------

``string`` is a |variadic|, `bidirectional`__, `extensible`__ |Integral Sequence Wrapper| of
characters that supports amortized constant-time insertion and removal of elements at both ends,
and linear-time insertion and removal of elements in the middle. The parameters to ``string``
are multi-character literals, giving a somewhat readable syntax for compile-time strings.
``string`` can also be an argument to the ``c_str`` metafunction, which generates a
null-terminated character array that facilitates interoperability with runtime string
processing routines.

__ `Bidirectional Sequence`_
__ `Extensible Sequence`_

Header
------

+-------------------+-------------------------------------------------------+
| Sequence form     | Header                                                |
+===================+=======================================================+
| Variadic          | ``#include <boost/mpl/string.hpp>``                   |
+-------------------+-------------------------------------------------------+

Model of
--------

* |Integral Sequence Wrapper|
* |Variadic Sequence|
* |Bidirectional Sequence|
* |Extensible Sequence|
* |Back Extensible Sequence|
* |Front Extensible Sequence|

Expression semantics
--------------------

In the following table, ``s`` is an instance of ``string``, ``pos`` and ``last`` are iterators 
into ``s``, ``r`` is a |Forward Sequence| of characters, ``n`` and ``x`` are |Integral Constant|\ s,
and |c1...cn| are arbitrary (multi-)characters.

+---------------------------------------+-----------------------------------------------------------+
| Expression                            | Semantics                                                 |
+=======================================+===========================================================+
| .. parsed-literal::                   | ``string`` of characters |c1...cn|; see                   |
|                                       | |Variadic Sequence|.                                      |
|    string<|c1...cn|>                  |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| .. parsed-literal::                   | Identical to ``string<``\ |c1...cn|\ ``>``;               |
|                                       | see |Variadic Sequence|.                                  |
|    string<|c1...cn|>::type            |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``begin<s>::type``                    | An iterator pointing to the beginning of ``s``;           |
|                                       | see |Bidirectional Sequence|.                             |
+---------------------------------------+-----------------------------------------------------------+
| ``end<s>::type``                      | An iterator pointing to the end of ``s``;                 |
|                                       | see |Bidirectional Sequence|.                             |
+---------------------------------------+-----------------------------------------------------------+
| ``size<s>::type``                     | The size of ``s``; see |Bidirectional Sequence|.          |
+---------------------------------------+-----------------------------------------------------------+
| ``empty<s>::type``                    | |true if and only if| the sequence is empty;              |
|                                       | see |Bidirectional Sequence|.                             |
+---------------------------------------+-----------------------------------------------------------+
| ``front<s>::type``                    | The first element in ``s``; see                           |
|                                       | |Bidirectional Sequence|.                                 |
+---------------------------------------+-----------------------------------------------------------+
| ``back<s>::type``                     | The last element in ``s``; see                            |
|                                       | |Bidirectional Sequence|.                                 |
+---------------------------------------+-----------------------------------------------------------+
| ``insert<s,pos,x>::type``             | A new ``string`` of following elements:                   |
|                                       | [``begin<s>::type``, ``pos``), ``x``,                     |
|                                       | [``pos``, ``end<s>::type``); see |Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``insert_range<s,pos,r>::type``       | A new ``string`` of following elements:                   |
|                                       | [``begin<s>::type``, ``pos``),                            |
|                                       | [``begin<r>::type``, ``end<r>::type``)                    |
|                                       | [``pos``, ``end<s>::type``); see |Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<s,pos>::type``                | A new ``string`` of following elements:                   |
|                                       | [``begin<s>::type``, ``pos``),                            |
|                                       | [``next<pos>::type``, ``end<s>::type``); see              |
|                                       | |Extensible Sequence|.                                    |
+---------------------------------------+-----------------------------------------------------------+
| ``erase<s,pos,last>::type``           | A new ``string`` of following elements:                   |
|                                       | [``begin<s>::type``, ``pos``),                            |
|                                       | [``last``, ``end<s>::type``); see |Extensible Sequence|.  |
+---------------------------------------+-----------------------------------------------------------+
| ``clear<s>::type``                    | An empty ``string``; see |Extensible Sequence|.           |
+---------------------------------------+-----------------------------------------------------------+
| ``push_back<s,x>::type``              | A new ``string`` of following elements:                   | 
|                                       | |begin/end<s>|, ``x``;                                    |
|                                       | see |Back Extensible Sequence|.                           |
+---------------------------------------+-----------------------------------------------------------+
| ``pop_back<s>::type``                 | A new ``string`` of following elements:                   |
|                                       | [``begin<s>::type``, ``prior< end<s>::type >::type``);    |
|                                       | see |Back Extensible Sequence|.                           |
+---------------------------------------+-----------------------------------------------------------+
| ``push_front<s,x>::type``             | A new ``string`` of following elements:                   |
|                                       | |begin/end<s>|, ``x``; see |Front Extensible Sequence|.   |
+---------------------------------------+-----------------------------------------------------------+
| ``pop_front<s>::type``                | A new ``string`` of following elements:                   |
|                                       | [``next< begin<s>::type >::type``, ``end<s>::type``);     |
|                                       | see |Front Extensible Sequence|.                          |
+---------------------------------------+-----------------------------------------------------------+
| ``c_str<s>::value``                   | A null-terminated byte string such that                   |
|                                       | ``c_str<s>::value[``\ *n*\ ``]`` is equal to the *n*\ -th |
|                                       | character in ``s``, and                                   |
|                                       | ``c_str<s>::value[size<s>::type::value]`` is ``'\0'``.    |
+---------------------------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::
   
    typedef mpl::string<'hell','o wo','rld'> hello;
    typedef mpl::push_back<hello, mpl::char_<'!'> >::type hello2;

    BOOST_ASSERT(0 == std::strcmp(mpl::c_str<hello2>::value, "hello world!"));


See also
--------

|Sequences|, |Variadic Sequence|, |Bidirectional Sequence|, |Extensible Sequence|, |Integral Sequence Wrapper|, |char_|, |c_str|


.. copyright:: Copyright ©  2009 Eric Niebler
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
