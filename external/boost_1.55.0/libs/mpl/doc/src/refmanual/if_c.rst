.. Metafunctions/Type Selection//if_c |20

if_c
====

Synopsis
--------

.. parsed-literal::
    
    template< 
          bool c
        , typename T1
        , typename T2
        >
    struct if_c
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns one of its two arguments, ``T1`` or ``T2``, depending on the value of
integral constant ``c``. ``if_c<c,t1,t2>::type`` is a shorcut notation for 
``if_< bool_<c>,t1,t2 >::type``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/if.hpp>


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``c``         | An integral constant              | A selection condition.                        |
+---------------+-----------------------------------+-----------------------------------------------+
| ``T1``, ``T2``| Any type                          | Types to select from.                         |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any integral constant ``c`` and arbitrary types ``t1``, ``t2``:


.. parsed-literal::

    typedef if_c<c,t1,t2>::type t;

:Return type:
    Any type.

:Semantics:
    Equivalent to ``typedef if_< bool_<c>,t1,t2 >::type t;`` 


Example
-------

.. parsed-literal::
    
    typedef if_c<true,char,long>::type t1;
    typedef if_c<false,char,long>::type t2;

    BOOST_MPL_ASSERT(( is_same<t1, char> ));
    BOOST_MPL_ASSERT(( is_same<t2, long> ));


.. parsed-literal::

    // allocates space for an object of class T on heap or "inplace"
    // depending on its size
    template< typename T > struct lightweight
    {
        // ...
        typedef typename if_c<
              sizeof(T) <= sizeof(T*)
            , inplace_storage<T>
            , heap_storage<T>
            >::type impl_t;

        impl_t impl;
    };


See also
--------

|Metafunctions|, |Integral Constant|, |if_|, |eval_if|, |bool_|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
