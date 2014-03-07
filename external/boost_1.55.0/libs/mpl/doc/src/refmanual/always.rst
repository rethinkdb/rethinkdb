.. Metafunctions/Miscellaneous//always |20

always
======

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename X
        >
    struct always
    {
        // |unspecified|
        // |...|
    };


Description
-----------

``always<X>`` specialization is a variadic |Metafunction Class| always returning the 
same type, ``X``, regardless of the number and types of passed arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/always.hpp>

Model of
--------

|Metafunction Class|


Parameters
----------

+---------------+-------------------+-----------------------------------+
| Parameter     | Requirement       | Description                       |
+===============+===================+===================================+
| ``X``         | Any type          | A type to be returned.            |
+---------------+-------------------+-----------------------------------+


Expression semantics
--------------------

For an arbitrary type ``x``:


.. parsed-literal::

    typedef always<x> f;

:Return type:
    |Metafunction Class|.

:Semantics:
    Equivalent to
    
    .. parsed-literal::
    
        struct f : bind< identity<_1>, x > {};


Example
-------

.. parsed-literal::
    
    typedef always<true\_> always_true;

    BOOST_MPL_ASSERT(( apply< always_true,false\_> ));
    BOOST_MPL_ASSERT(( apply< always_true,false\_,false\_ > ));
    BOOST_MPL_ASSERT(( apply< always_true,false\_,false\_,false\_ > ));


See also
--------

|Metafunctions|, |Metafunction Class|, |identity|, |bind|, |apply|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
