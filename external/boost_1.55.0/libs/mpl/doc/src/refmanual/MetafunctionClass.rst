.. Metafunctions/Concepts//Metafunction Class |20

Metafunction Class
==================

Summary
-------

A *metafunction class* is a certain form of metafunction representation 
that enables higher-order metaprogramming. More precisely, it's a class
with a publicly-accessible nested |metafunction| called ``apply``. 
Correspondingly, a metafunction class invocation is defined as invocation
of its nested ``apply`` metafunction.


Expression requirements
-----------------------

|In the following table...| ``f`` is a |Metafunction Class|.

+-------------------------------+---------------------------+---------------------------+
| Expression                    | Type                      | Complexity                |
+===============================+===========================+===========================+
| ``f::apply::type``            | Any type                  | Unspecified.              |
+-------------------------------+---------------------------+---------------------------+
| ``f::apply<>::type``          | Any type                  | Unspecified.              |
+-------------------------------+---------------------------+---------------------------+
| ``f::apply<a1,...an>::type``  | Any type                  | Unspecified.              |
+-------------------------------+---------------------------+---------------------------+


Expression semantics
--------------------

.. parsed-literal::

    typedef f::apply::type x;

:Precondition:
    ``f`` is a nullary |Metafunction Class|; ``f::apply::type`` is a *type-name*.

:Semantics:
    ``x`` is the result of the metafunction class invocation.


.. ...................................................................................

.. parsed-literal::

    typedef f::apply<>::type x;

:Precondition:
    ``f`` is a nullary |Metafunction Class|; ``f::apply<>::type`` is a *type-name*.

:Semantics:
    ``x`` is the result of the metafunction class invocation.


.. ...................................................................................

.. parsed-literal::

    typedef f::apply<a1,\ |...|\ a\ *n*\>::type x;

:Precondition:
    ``f`` is an *n*-ary metafunction class; ``apply`` is a |Metafunction|.
    
:Semantics:
    ``x`` is the result of the metafunction class
    invocation with the actual arguments |a1...an|.


Models
------

* |always|
* |arg|
* |quote|
* |numeric_cast|
* |unpack_args|


See also
--------

|Metafunctions|, |Metafunction|, |Lambda Expression|, |Invocation|, |apply_wrap|, |bind|, |quote|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
