.. Metafunctions/Miscellaneous//inherit |30

inherit
=======

Synopsis
--------

.. parsed-literal::

    template<
          typename T1, typename T2
        >
    struct inherit\ ``2``
    {
        typedef |unspecified| type;
    };

    |...|

    template<
          typename T1, typename T2,\ |...| typename T\ *n*
        >
    struct inherit\ *n*
    {
        typedef |unspecified| type;
    };
    
    template<
          typename T1
        , typename T2
        |...|
        , typename T\ *n* = |unspecified|
        >
    struct inherit
    {
        typedef |unspecified| type;
    };


Description
-----------

Returns an unspecified class type publically derived from |T1...Tn|.
Guarantees that derivation from |empty_base| is always a no-op, 
regardless of the position and number of |empty_base| classes in 
|T1...Tn|.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/inherit.hpp>


Model of
--------

|Metafunction|


Parameters
----------

+---------------+-------------------+-----------------------------------+
| Parameter     | Requirement       | Description                       |
+===============+===================+===================================+
| |T1...Tn|     | A class type      | Classes to derived from.          |
+---------------+-------------------+-----------------------------------+


Expression semantics
--------------------

For artibrary class types |t1...tn|:

.. parsed-literal::

    typedef inherit2<t1,t2>::type r; 

:Return type:
    A class type.

:Precondition:
    ``t1`` and ``t2`` are complete types.

:Semantics:
    If both ``t1`` and ``t2`` are identical to ``empty_base``, equivalent to
        
    .. parsed-literal::
    
        typedef empty_base r;


    otherwise, if ``t1`` is identical to ``empty_base``, equivalent to

    .. parsed-literal::

        typedef t2 r;


    otherwise, if ``t2`` is identical to ``empty_base``, equivalent to

    .. parsed-literal::

        typedef t1 r;


    otherwise equivalent to

    .. parsed-literal::

        struct r : t1, t2 {};

.. ...........................................................................

.. parsed-literal::

    typedef inherit\ *n*\<t1,t2,\ |...|\ t\ *n*\ >::type r; 

:Return type:
    A class type.

:Precondition:
    |t1...tn| are complete types.

:Semantics:
    Equivalent to

    .. parsed-literal::
    
        struct r
            : inherit\ ``2``\<
                  inherit\ *n-1*\<t1,t2,\ |...|\ t\ *n-1*\>::type
                , t\ *n*
                >
        {
        };


.. ...........................................................................


.. parsed-literal::

    typedef inherit<t1,t2,\ |...|\ t\ *n*\ >::type r; 

:Precondition:
    |t1...tn| are complete types.

:Return type:
    A class type.

:Semantics:
    Equivalent to

    .. parsed-literal::

        typedef inherit\ *n*\<t1,t2,\ |...|\ t\ *n*\ >::type r; 



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    struct udt1 { int n; };
    struct udt2 {};

    typedef inherit<udt1,udt2>::type r1;
    typedef inherit<empty_base,udt1>::type r2;
    typedef inherit<empty_base,udt1,empty_base,empty_base>::type r3;
    typedef inherit<udt1,empty_base,udt2>::type r4;
    typedef inherit<empty_base,empty_base>::type r5;

    BOOST_MPL_ASSERT(( is_base_and_derived< udt1, r1> ));
    BOOST_MPL_ASSERT(( is_base_and_derived< udt2, r1> ));
    BOOST_MPL_ASSERT(( is_same< r2, udt1> ));    
    BOOST_MPL_ASSERT(( is_same< r3, udt1 > ));
    BOOST_MPL_ASSERT(( is_base_and_derived< udt1, r4 > ));
    BOOST_MPL_ASSERT(( is_base_and_derived< udt2, r4 > ));
    BOOST_MPL_ASSERT(( is_same< r5, empty_base > ));


See also
--------

|Metafunctions|, |empty_base|, |inherit_linearly|, |identity|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
