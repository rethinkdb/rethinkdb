.. Metafunctions/Composition and Argument Binding//quote |40

quote
=====

Synopsis
--------

.. parsed-literal::

    template<
          template< typename P1 > class F
        , typename Tag = |unspecified|
        >
    struct quote1
    {
        // |unspecified|
        // |...|
    };    

    |...|
    
    template<
          template< typename P1,\ |...| typename P\ *n* > class F
        , typename Tag = |unspecified|
        >
    struct quote\ *n*
    {
        // |unspecified|
        // |...|
    };    


Description
-----------

``quote``\ *n* is a higher-order primitive that wraps an *n*-ary |Metafunction| to create 
a corresponding |Metafunction Class|.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/quote.hpp>


Model of
--------

|Metafunction Class|


Parameters
----------

+---------------+-----------------------+-----------------------------------------------+
| Parameter     | Requirement           | Description                                   |
+===============+=======================+===============================================+
| ``F``         | |Metafunction|        | A metafunction to wrap.                       |
+---------------+-----------------------+-----------------------------------------------+
| ``Tag``       | Any type              | A tag determining wrap semantics.             |
+---------------+-----------------------+-----------------------------------------------+


Expression semantics
--------------------

For any *n*-ary |Metafunction| ``f`` and arbitrary type ``tag``:


.. parsed-literal::

    typedef quote\ *n*\ <f> g;
    typedef quote\ *n*\ <f,tag> g;

:Return type:
    |Metafunction Class|

:Semantics:
    Equivalent to
    
    .. parsed-literal::
    
        struct g
        {
            template< typename A1,\ |...| typename A\ *n* > struct apply
                : f<A1,\ |...|\ A\ *n*\ >
            {
            };
        };
        
    if ``f<A1,...An>`` has a nested type member ``::type``, and to

    .. parsed-literal::
    
        struct g
        {
            template< typename A1,\ |...| typename A\ *n* > struct apply
            {
                typedef f<A1,\ |...|\ A\ *n*\ > type;
            };
        };

    otherwise.


Example
-------

.. parsed-literal::

    template< typename T > struct f1
    {
        typedef T type;
    };

    template<
        typename T1, typename T2, typename T3, typename T4, typename T5
        >
    struct f5
    {
        // no 'type' member!
    };

    typedef quote\ ``1``\<f1>::apply<int>::type t1;
    typedef quote\ ``5``\<f5>::apply<char,short,int,long,float>::type t5;
    
    BOOST_MPL_ASSERT(( is_same< t1, int > ));
    BOOST_MPL_ASSERT(( is_same< t5, f5<char,short,int,long,float> > ));


See also
--------

|Composition and Argument Binding|, |Invocation|, |bind|, |lambda|, |protect|, |apply|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
