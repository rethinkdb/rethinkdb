.. Metafunctions/Composition and Argument Binding//lambda |20

lambda
======

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename X
        , typename Tag = |unspecified|
        >
    struct lambda
    {
        typedef |unspecified| type;
    };



Description
-----------

If ``X`` is a |placeholder expression|, transforms ``X`` into a corresponding 
|Metafunction Class|, otherwise ``X`` is returned unchanged.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/lambda.hpp>


Parameters
----------

+---------------+-----------------------+-----------------------------------------------+
| Parameter     | Requirement           | Description                                   |
+===============+=======================+===============================================+
| ``X``         | Any type              | An expression to transform.                   |
+---------------+-----------------------+-----------------------------------------------+
| ``Tag``       | Any type              | A tag determining transform semantics.        |
+---------------+-----------------------+-----------------------------------------------+

Expression semantics
--------------------

For arbitrary types ``x`` and ``tag``:


.. parsed-literal::

    typedef lambda<x>::type f;

:Return type:
    |Metafunction Class|.

:Semantics:
    If ``x`` is a |placeholder expression| in a general form ``X<a1,...an>``, where
    ``X`` is a class template and ``a1``,... ``an`` are arbitrary types, equivalent 
    to

    .. parsed-literal::
    
        typedef protect< bind<
              quote\ *n*\ <X>
            , lambda<a1>::type,\ |...| lambda<a\ *n*\ >::type
            > > f;
    
    otherwise, ``f`` is identical to ``x``.

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. parsed-literal::

    typedef lambda<x,tag>::type f;

:Return type:
    |Metafunction Class|.

:Semantics:
    If ``x`` is a |placeholder expression| in a general form ``X<a1,...an>``, where
    ``X`` is a class template and ``a1``,... ``an`` are arbitrary types, equivalent 
    to

    .. parsed-literal::
    
        typedef protect< bind<
              quote\ *n*\ <X,tag>
            , lambda<a1,tag>::type,\ |...| lambda<a\ *n*\ ,tag>::type
            > > f;
    
    otherwise, ``f`` is identical to ``x``.


Example
-------

.. parsed-literal::
    
    template< typename N1, typename N2 > struct int_plus
        : int_<( N1::value + N2::value )>
    {
    };
    
    typedef lambda< int_plus<_1, int_<42> > >::type f1;
    typedef bind< quote\ ``2``\ <int_plus>, _1, int_<42> > f2;

    typedef f1::apply<42>::type r1;
    typedef f2::apply<42>::type r2;

    BOOST_MPL_ASSERT_RELATION( r1::value, ==, 84 );
    BOOST_MPL_ASSERT_RELATION( r2::value, ==, 84 );


See also
--------

|Composition and Argument Binding|, |Invocation|, |Placeholders|, |bind|, |quote|, |protect|, |apply|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
