.. Metafunctions/Invocation//apply_wrap |20

apply_wrap
==========

Synopsis
--------

.. parsed-literal::

    template< 
          typename F
        >
    struct apply_wrap0
    {
        typedef |unspecified| type;
    };

    template< 
          typename F, typename A1
        >
    struct apply_wrap1
    {
        typedef |unspecified| type;
    };
    
    |...|
    
    template< 
          typename F, typename A1,\ |...| typename An
        >
    struct apply_wrap\ *n*
    {
        typedef |unspecified| type;
    };



Description
-----------

Invokes a |Metafunction Class| ``F`` with arguments ``A1``,... ``An``. 

In essence, ``apply_wrap`` forms are nothing more than syntactic wrappers around 
``F::apply<A1,... An>::type`` / ``F::apply::type`` expressions (hence the name). 
They provide a more concise notation and higher portability than their 
underlaying constructs at the cost of an extra template instantiation.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/apply_wrap.hpp>


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``F``         | |Metafunction Class|              | A metafunction class to invoke.               |
+---------------+-----------------------------------+-----------------------------------------------+
| |A1...An|     | Any type                          | Invocation arguments.                         |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Metafunction Class| ``f`` and arbitrary types |a1...an|:


.. parsed-literal::

    typedef apply_wrap\ *n*\ <f,a1,\ |...|\ an>::type t;

:Return type:
    Any type.

:Semantics:
    If ``n > 0``, equivalent to ``typedef f::apply<a1,... an>::type t;``,
    otherwise equivalent to either ``typedef f::apply::type t;`` or
    ``typedef f::apply<>::type t;`` depending on whether ``f::apply`` is 
    a class or a class template.


Example
-------

.. parsed-literal::

    struct f0
    {
        template< typename T = int > struct apply
        {
            typedef char type;
        };
    };
        
    struct g0
    {
        struct apply { typedef char type; };
    };

    struct f2
    {
        template< typename T1, typename T2 > struct apply
        {
            typedef T2 type;
        };
    };

    
    typedef apply_wrap\ ``0``\ < f0 >::type r1;
    typedef apply_wrap\ ``0``\ < g0 >::type r2;
    typedef apply_wrap\ ``2``\ < f2,int,char >::type r3;

    BOOST_MPL_ASSERT(( is_same<r1,char> ));
    BOOST_MPL_ASSERT(( is_same<r2,char> ));
    BOOST_MPL_ASSERT(( is_same<r3,char> ));


See also
--------

|Metafunctions|, |Invocation|, |apply|, |lambda|, |quote|, |bind|, |protect|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
