.. Metafunctions/Concepts//Metafunction |10

Metafunction
============

Description
-----------

.. _`nullary-metafunction`:

A *metafunction* is a class or a class template that represents a 
function invocable at compile-time. An non-nullary metafunction is 
invoked by instantiating the class template with particular 
template parameters (metafunction arguments); the result of the 
metafunction application is accessible through the instantiation's 
nested ``type`` typedef. All metafunction's arguments must be types 
(i.e. only *type template parameters* are allowed). A metafunction 
can have a variable number of parameters. A *nullary metafunction* is 
represented as a (template) class with a nested ``type`` typename 
member. 

.. |nullary metafunction| replace:: `nullary-metafunction`_


Expression requirements
-----------------------

|In the following table...| ``f`` is a |Metafunction|.

+-------------------------------+-----------------------+---------------------------+
| Expression                    | Type                  | Complexity                |
+===============================+=======================+===========================+
| ``f::type``                   | Any type              | Unspecified.              |
+-------------------------------+-----------------------+---------------------------+
| ``f<>::type``                 | Any type              | Unspecified.              |
+-------------------------------+-----------------------+---------------------------+
| ``f<a1,..,an>::type``         | Any type              | Unspecified.              |
+-------------------------------+-----------------------+---------------------------+


Expression semantics
--------------------

.. parsed-literal::

    typedef f::type x;

:Precondition:
    ``f`` is a nullary |Metafunction|; ``f::type`` is a *type-name*.

:Semantics:
    ``x`` is the result of the metafunction invocation.


.. ...................................................................................

.. parsed-literal::

    typedef f<>::type x;

:Precondition:
    ``f`` is a nullary |Metafunction|; ``f<>::type`` is a *type-name*.

:Semantics:
    ``x`` is the result of the metafunction invocation.


.. ...................................................................................

.. parsed-literal::

    typedef f<a1,\ |...| \a\ *n*\>::type x;

:Precondition:
    ``f`` is an *n*-ary |Metafunction|; |a1...an| are types; 
    ``f<a1,...an>::type`` is a *type-name*.
    
:Semantics:
    ``x`` is the result of the metafunction invocation 
    with the actual arguments |a1...an|.


Models
------

* |identity|
* |plus|
* |begin|
* |insert|
* |fold|


See also
--------

|Metafunctions|, |Metafunction Class|, |Lambda Expression|, |Invocation|, |apply|, |lambda|, |bind|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
