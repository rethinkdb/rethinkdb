.. Metafunctions/Composition and Argument Binding//arg |50

arg
===

Synopsis
--------

.. parsed-literal::
   
    template< int n > struct arg;

    template<> struct arg<1>
    {
        template< typename A1,\ |...| typename A\ *n* = |unspecified| >
        struct apply
        {
            typedef A1 type;
        };
    };

    |...|

    template<> struct arg<\ *n*\>
    {
        template< typename A1,\ |...| typename A\ *n* >
        struct apply
        {
            typedef A\ *n* type;
        };
    };


Description
-----------

``arg<n>`` specialization is a |Metafunction Class| that return the ``n``\ th of its arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/arg.hpp>


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``n``         | An integral constant              | A number of argument to return.               |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any integral constant ``n`` in the range [1, |BOOST_MPL_LIMIT_METAFUNCTION_ARITY|\] and
arbitrary types |a1...an|:

.. parsed-literal::

    typedef apply_wrap\ *n*\< arg<\ *n*\ >,a1,\ |...|\a\ *n* >::type x;

:Return type:
    A type.

:Semantics:
    ``x`` is identical to ``an``.


Example
-------

.. parsed-literal::

    typedef apply_wrap\ ``5``\< arg<1>,bool,char,short,int,long >::type t1;
    typedef apply_wrap\ ``5``\< arg<3>,bool,char,short,int,long >::type t3;
    
    BOOST_MPL_ASSERT(( is_same< t1, bool > ));
    BOOST_MPL_ASSERT(( is_same< t3, short > ));


See also
--------

|Composition and Argument Binding|, |Placeholders|, |lambda|, |bind|, |apply|, |apply_wrap|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
