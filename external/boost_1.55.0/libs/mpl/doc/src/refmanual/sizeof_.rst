.. Metafunctions/Miscellaneous//sizeof_ |100

sizeof\_
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename X
        >
    struct sizeof\_
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of a ``sizeof(X)`` expression wrapped into an 
|Integral Constant| of the corresponding type, ``std::size_t``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/sizeof.hpp>


Model of
--------

|Metafunction|


Parameters
----------

+---------------+-------------------+-------------------------------------------+
| Parameter     | Requirement       | Description                               |
+===============+===================+===========================================+
| ``X``         | Any type          | A type to compute the ``sizeof`` for.     |
+---------------+-------------------+-------------------------------------------+


Expression semantics
--------------------

For an arbitrary type ``x``:


.. parsed-literal::

    typedef sizeof_<x>::type n; 


:Return type:
    |Integral Constant|.

:Precondition:
    ``x`` is a complete type.

:Semantics:
    Equivalent to
        
    .. parsed-literal::
    
        typedef size_t< sizeof(x) > n;



Complexity
----------

Constant time. 


Example
-------

.. parsed-literal::
    
    struct udt { char a[100]; };

    BOOST_MPL_ASSERT_RELATION( sizeof_<char>::value, ==, sizeof(char) );
    BOOST_MPL_ASSERT_RELATION( sizeof_<int>::value, ==, sizeof(int) );
    BOOST_MPL_ASSERT_RELATION( sizeof_<double>::value, ==, sizeof(double) );
    BOOST_MPL_ASSERT_RELATION( sizeof_<udt>::value, ==, sizeof(my) );


See also
--------

|Metafunctions|, |Integral Constant|, |size_t|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
