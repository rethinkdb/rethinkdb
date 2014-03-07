.. Data Types/Concepts//Integral Constant

Integral Constant
=================

Description
-----------

An |Integral Constant| is a holder class for a compile-time value of an
integral type. Every |Integral Constant| is also a nullary |Metafunction|, 
returning itself. An integral constant *object* is implicitly convertible to the 
corresponding run-time value of the wrapped integral type.

Expression requirements
-----------------------

|In the following table...| ``n`` is a model of |Integral Constant|.

+-----------------------------------+---------------------------------------+---------------------------+
| Expression                        | Type                                  | Complexity                |
+===================================+=======================================+===========================+
| ``n::tag``                        | ``integral_c_tag``                    | Constant time.            |
+-----------------------------------+---------------------------------------+---------------------------+
| ``n::value_type``                 | An integral type                      | Constant time.            |
+-----------------------------------+---------------------------------------+---------------------------+
| ``n::value``                      | An integral constant expression       | Constant time.            |
+-----------------------------------+---------------------------------------+---------------------------+
| ``n::type``                       | |Integral Constant|                   | Constant time.            |
+-----------------------------------+---------------------------------------+---------------------------+
| ``next<n>::type``                 | |Integral Constant|                   | Constant time.            |
+-----------------------------------+---------------------------------------+---------------------------+
| ``prior<n>::type``                | |Integral Constant|                   | Constant time.            |
+-----------------------------------+---------------------------------------+---------------------------+
| ``n::value_type const c = n()``   |                                       | Constant time.            |
+-----------------------------------+---------------------------------------+---------------------------+


Expression semantics
--------------------

+---------------------------------------+-----------------------------------------------------------+
| Expression                            | Semantics                                                 |
+=======================================+===========================================================+
| ``n::tag``                            | ``n``\ 's tag type; ``n::tag::value`` is ``n``\ 's        |
|                                       | *conversion rank*.                                        |
+---------------------------------------+-----------------------------------------------------------+
| ``n::value_type``                     | A cv-unqualified type of ``n::value``.                    |
+---------------------------------------+-----------------------------------------------------------+
| ``n::value``                          | The value of the wrapped integral constant.               |
+---------------------------------------+-----------------------------------------------------------+
| ``n::type``                           | ``is_same<n::type,n>::value == true``.                    |
+---------------------------------------+-----------------------------------------------------------+
| ``next<n>::type``                     | An |Integral Constant| ``c`` of type ``n::value_type``    |
|                                       | such that ``c::value == n::value + 1``.                   |
+---------------------------------------+-----------------------------------------------------------+
| ``prior<n>::type``                    | An |Integral Constant| ``c`` of type ``n::value_type``    |
|                                       | such that ``c::value == n::value - 1``.                   |
+---------------------------------------+-----------------------------------------------------------+
| ``n::value_type const c = n()``       | ``c == n::value``.                                        |
+---------------------------------------+-----------------------------------------------------------+


Models
------

* |bool_|
* |int_|
* |long_|
* |char_|
* |integral_c|


See also
--------

|Data Types|, |Integral Sequence Wrapper|, |integral_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
