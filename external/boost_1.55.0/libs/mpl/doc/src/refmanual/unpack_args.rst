.. Metafunctions/Invocation//unpack_args |30

unpack_args
===========

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename F
        >
    struct unpack_args
    {
        // |unspecified|
        // |...|
    };


Description
-----------

A higher-order primitive transforming an *n*-ary |Lambda Expression| ``F`` into
an unary |Metafunction Class| ``g`` accepting a single sequence of *n* arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/unpack_args.hpp>


Model of
--------

|Metafunction Class|


Parameters
----------

+---------------+-----------------------+-------------------------------------------+
| Parameter     | Requirement           | Description                               |
+===============+=======================+===========================================+
| ``F``         | |Lambda Expression|   | A lambda expression to adopt.             |
+---------------+-----------------------+-------------------------------------------+


Expression semantics
--------------------

For an arbitrary |Lambda Expression| ``f``, and arbitrary types |a1...an|:


.. parsed-literal::

    typedef unpack_args<f> g;

:Return type:
    |Metafunction Class|.

:Semantics:
    ``g`` is a unary |Metafunction Class| such that
    
    .. parsed-literal::

        apply_wrap\ *n*\ < g, vector<a1,\ |...|\ a\ *n*\ > >::type
       
    is identical to

    .. parsed-literal::

        apply<F,a1,\ |...|\ a\ *n*\ >::type


Example
-------

.. parsed-literal::
    
    BOOST_MPL_ASSERT(( apply< 
          unpack_args< is_same<_1,_2> >
        , vector<int,int>
        > ));


See also
--------

|Metafunctions|, |Lambda Expression|, |Metafunction Class|, |apply|, |apply_wrap|, |bind|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
