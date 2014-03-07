.. Metafunctions/Composition and Argument Binding//protect |60

protect
=======

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename F
        >
    struct protect
    {
        // |unspecified|
        // |...|
    };



Description
-----------

``protect`` is an identity wrapper for a |Metafunction Class| that prevents
its argument from being recognized as a |bind expression|.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/protect.hpp>


Parameters
----------

+---------------+---------------------------+---------------------------------------+
| Parameter     | Requirement               | Description                           |
+===============+===========================+=======================================+
| ``F``         | |Metafunction Class|      | A metafunction class to wrap.         |
+---------------+---------------------------+---------------------------------------+


Expression semantics
--------------------

For any |Metafunction Class| ``f``:


.. parsed-literal::

    typedef protect<f> g;

:Return type:
    |Metafunction Class|.

:Semantics:
    If ``f`` is a |bind expression|, equivalent to
    
    .. parsed-literal::

        struct g
        {
            template< 
                  typename U1 = |unspecified|\,\ |...| typename U\ *n* = |unspecified|
                >
            struct apply
                : apply_wrap\ *n*\<f,U1,\ |...|\ U\ *n*\ >
            {
            };
        };
    
    otherwise equivalent to ``typedef f g;``.


Example
-------

.. parsed-literal::
    
    struct f
    {
        template< typename T1, typename T2 > struct apply
        {
            typedef T2 type;
        };
    };
     
    typedef bind< quote\ ``3``\<if\_>,_1,_2,bind<f,_1,_2> > b1;
    typedef bind< quote\ ``3``\<if\_>,_1,_2,protect< bind<f,_1,_2> > > b2;
     
    typedef apply_wrap\ ``2``\< b1,false\_,char >::type r1;
    typedef apply_wrap\ ``2``\< b2,false\_,char >::type r2;
     
    BOOST_MPL_ASSERT(( is_same<r1,char> ));
    BOOST_MPL_ASSERT(( is_same<r2,protect< bind<f,_1,_2> > > ));


See also
--------

|Composition and Argument Binding|, |Invocation|, |bind|, |quote|, |apply_wrap|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
