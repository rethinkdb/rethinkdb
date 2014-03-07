.. Metafunctions/Invocation//apply |10

apply
=====

Synopsis
--------

.. parsed-literal::

    template< 
          typename F
        >
    struct apply0
    {
        typedef |unspecified| type;
    };

    template< 
          typename F, typename A1
        >
    struct apply1
    {
        typedef |unspecified| type;
    };
    
    |...|
    
    template< 
          typename F, typename A1,\ |...| typename An
        >
    struct apply\ *n*
    {
        typedef |unspecified| type;
    };
    
    template< 
          typename F
        , typename A1 = |unspecified|
        |...|
        , typename An = |unspecified|
        >
    struct apply
    {
        typedef |unspecified| type;
    };



Description
-----------

Invokes a |Metafunction Class| or a |Lambda Expression| ``F`` with arguments ``A1``,... ``An``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/apply.hpp>


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``F``         | |Lambda Expression|               | An expression to invoke.                      |
+---------------+-----------------------------------+-----------------------------------------------+
| |A1...An|     | Any type                          | Invocation arguments.                         |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Lambda Expression| ``f`` and arbitrary types ``a1``,... ``an``:


.. parsed-literal::

    typedef apply\ *n*\<f,a1,\ |...|\ a\ *n*\>::type t;
    typedef apply<f,a1,\ |...|\ a\ *n*\>::type t;

:Return type:
    Any type.

:Semantics:
    Equivalent to ``typedef apply_wrap``\ *n*\ ``< lambda<f>::type,a1,... an>::type t;``.


Example
-------

.. parsed-literal::

    template< typename N1, typename N2 > struct int_plus
        : int_<( N1::value + N2::value )>
    {
    };
    
    typedef apply< int_plus<_1,_2>, int_<2>, int_<3> >::type r1;
    typedef apply< quote\ ``2``\ <int_plus>, int_<2>, int_<3> >::type r2;
    
    BOOST_MPL_ASSERT_RELATION( r1::value, ==, 5 );
    BOOST_MPL_ASSERT_RELATION( r2::value, ==, 5 );


See also
--------

|Metafunctions|, |apply_wrap|, |lambda|, |quote|, |bind|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
