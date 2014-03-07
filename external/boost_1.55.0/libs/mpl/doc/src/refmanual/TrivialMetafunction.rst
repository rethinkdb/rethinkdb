.. Metafunctions/Concepts//Trivial Metafunction |70

Trivial Metafunction
====================

Description
-----------

A |Trivial Metafunction| accepts a single argument of a class type ``x`` and 
returns the ``x``\ 's nested type member ``x::name``, where ``name`` is
a placeholder token for the actual member's name accessed by a specific 
metafunction's instance. By convention, all `trivial metafunctions`__ in MPL 
are named after the members they provide assess to. For instance, a |Trivial
Metafunction| named ``first`` reaches for the ``x``\ 's nested member 
``::first``.

__ `Trivial Metafunctions Summary`_


Expression requirements
-----------------------

|In the following table...| ``name`` is placeholder token for the names of 
the |Trivial Metafunction| itself and the accessed member, and ``x`` is
a class type such that ``x::name`` is a valid *type-name*.

+---------------------------+-------------------+---------------------------+
| Expression                | Type              | Complexity                |
+===========================+===================+===========================+
| ``name<x>::type``         | Any type          | Constant time.            |
+---------------------------+-------------------+---------------------------+


Expression semantics
--------------------

.. parsed-literal::

    typedef name<x>::type r;

:Precondition:
    ``x::name`` is a valid *type-name*.

:Semantics:
    ``is_same<r,x::name>::value == true``.


Models
------

* |first|
* |second|
* |base|


See also
--------

|Metafunctions|, |Trivial Metafunctions|, |identity|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
