.. Metafunctions/Composition and Argument Binding//bind |30

bind
====

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename F
        >
    struct bind0
    {
        // |unspecified|
        // |...|
    };

    template< 
          typename F, typename A1
        >
    struct bind1
    {
        // |unspecified|
        // |...|
    };
    
    |...|
    
    template< 
          typename F, typename A1,\ |...| typename An
        >
    struct bind\ *n*
    {
        // |unspecified|
        // |...|
    };
    
    template< 
          typename F
        , typename A1 = |unspecified|
        |...|
        , typename An = |unspecified|
        >
    struct bind
    {
        // |unspecified|
        // |...|
    };


Description
-----------

``bind`` is a higher-order primitive for |Metafunction Class| composition 
and argument binding. In essence, it's a compile-time counterpart of 
the similar run-time functionality provided by |Boost.Bind| and |Boost.Lambda|
libraries.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/bind.hpp>


Model of
--------

|Metafunction Class|


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``F``         | |Metafunction Class|              | An metafunction class to perform binding on.  |
+---------------+-----------------------------------+-----------------------------------------------+
| |A1...An|     | Any type                          | Arguments to bind.                            |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Metafunction Class| ``f`` and arbitrary types |a1...an|:

.. parsed-literal::

    typedef bind<f,a1,...a\ *n*\ > g;
    typedef bind\ *n*\ <f,a1,...a\ *n*\ > g;

:Return type:
    |Metafunction Class|

.. _`bind semantics`:

:Semantics:
    Equivalent to 

    .. parsed-literal::

        struct g
        {
            template< 
                  typename U1 = |unspecified|
                |...|
                , typename U\ *n* = |unspecified|
                >
            struct apply
                : apply_wrap\ *n*\ <
                      typename h0<f,U1,\ |...|\ U\ *n*>::type 
                    , typename h1<a1,U1,\ |...|\ U\ *n*>::type 
                    |...|
                    , typename h\ *n*\ <a\ *n*\ ,U1,\ |...|\ U\ *n*>::type 
                    >
            {
            };
        };

    where ``h``\ *k* is equivalent to 
    
    .. parsed-literal::
    
        template< typename X, typename U1,\ |...| typename U\ *n* > struct h\ *k*
            : apply_wrap\ *n*\ <X,U1,\ |...|\ U\ *n*>
        {
        };

    if ``f`` or ``a``\ *k* is a |bind expression| or a |placeholder|, and
    
    .. parsed-literal::
    
        template< typename X, typename U1,\ |...| typename U\ *n* > struct h\ *k*
        {
            typedef X type;
        };
    
    otherwise. |Note:| Every ``n``\th appearance of the `unnamed placeholder`__ 
    in the ``bind<f,a1,...an>`` specialization is replaced with the corresponding
    numbered placeholder ``_``\ *n* |-- end note|

__ `Placeholders`_


Example
-------

.. parsed-literal::
    
    struct f1
    {
        template< typename T1 > struct apply
        {
            typedef T1 type;
        };
    };

    struct f5
    {
        template< typename T1, typename T2, typename T3, typename T4, typename T5 >
        struct apply
        {
            typedef T5 type;
        };
    };
    
    typedef apply_wrap\ ``1``\< 
          bind\ ``1``\<f1,_1>
        , int 
        >::type r11;
    
    typedef apply_wrap\ ``5``\< 
          bind\ ``1``\<f1,_5>
        , void,void,void,void,int 
        >::type r12;
    
    BOOST_MPL_ASSERT(( is_same<r11,int> ));
    BOOST_MPL_ASSERT(( is_same<r12,int> ));
    
    typedef apply_wrap\ ``5``\< 
          bind\ ``5``\<f5,_1,_2,_3,_4,_5>
        , void,void,void,void,int 
        >::type r51;
    
    typedef apply_wrap\ ``5``\<
          bind\ ``5``\<f5,_5,_4,_3,_2,_1>
        , int,void,void,void,void
        >::type r52;
    
    BOOST_MPL_ASSERT(( is_same<r51,int> ));
    BOOST_MPL_ASSERT(( is_same<r52,int> ));


See also
--------

|Composition and Argument Binding|, |Invocation|, |Placeholders|, |lambda|, |quote|, 
|protect|, |apply|, |apply_wrap|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
