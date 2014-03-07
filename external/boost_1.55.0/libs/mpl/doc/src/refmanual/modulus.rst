.. Metafunctions/Arithmetic Operations//modulus |50

modulus
=======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T1
        , typename T2
        >
    struct modulus
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the modulus of its arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/modulus.hpp>
    #include <boost/mpl/arithmetic.hpp>


Model of
--------

|Numeric Metafunction|


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| ``T1``, ``T2``| |Integral Constant|       | Operation's arguments.                        |
+---------------+---------------------------+-----------------------------------------------+

|Note:| |numeric metafunction note| |-- end note|


Expression semantics
--------------------

For any |Integral Constant|\ s ``c1`` and ``c2``:


.. parsed-literal::

    typedef modulus<c1,c2>::type r;

:Return type:
    |Integral Constant|.

:Precondition:
    ``c2::value != 0`` 

:Semantics:
    Equivalent to 
        
    .. parsed-literal::
    
        typedef integral_c<
              typeof(c1::value % c2::value)
            , ( c1::value % c2::value )
            > r;


.. ..........................................................................

.. parsed-literal::

    typedef modulus<c1,c2> r;

:Return type:
    |Integral Constant|.

:Precondition:
    ``c2::value != 0`` 
 
:Semantics:
    Equivalent to 
    
    .. parsed-literal::
    
        struct r : modulus<c1,c2>::type {};


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef modulus< int_<10>, long_<3> >::type r;
    BOOST_MPL_ASSERT_RELATION( r::value, ==, 1 );
    BOOST_MPL_ASSERT(( is_same< r::value_type, long > ));



See also
--------

|Metafunctions|, |Numeric Metafunction|, |numeric_cast|, |divides|, |times|, |plus|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
