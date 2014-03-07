.. Metafunctions/Bitwise Operations//shift_right

shift_right
===========

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename T
        , typename Shift
        >
    struct shift_right
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of bitwise *shift right* (``>>``) operation on ``T``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/shift_right.hpp>
    #include <boost/mpl/bitwise.hpp>


Model of
--------

|Numeric Metafunction|


Parameters
----------

+---------------+-------------------------------+---------------------------+
| Parameter     | Requirement                   | Description               |
+===============+===============================+===========================+
| ``T``         | |Integral Constant|           | A value to shift.         |
+---------------+-------------------------------+---------------------------+
| ``Shift``     | Unsigned |Integral Constant|  | A shift distance.         |
+---------------+-------------------------------+---------------------------+

|Note:| |numeric metafunction note| |-- end note|


Expression semantics
--------------------

For arbitrary |Integral Constant| ``c`` and unsigned |Integral Constant| ``shift``:


.. parsed-literal::

    typedef shift_right<c,shift>::type r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 
        
    .. parsed-literal::
    
        typedef integral_c<
              c::value_type
            , ( c::value >> shift::value )
            > r;

.. ..........................................................................

.. parsed-literal::

    typedef shift_right<c,shift> r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to
    
    .. parsed-literal::

        struct r : shift_right<c,shift>::type {};


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::

    typedef integral_c<unsigned,0> u0;
    typedef integral_c<unsigned,1> u1;
    typedef integral_c<unsigned,2> u2;
    typedef integral_c<unsigned,8> u8;
    
    BOOST_MPL_ASSERT_RELATION( (shift_right<u0,u0>::value), ==, 0 );
    BOOST_MPL_ASSERT_RELATION( (shift_right<u1,u0>::value), ==, 1 );
    BOOST_MPL_ASSERT_RELATION( (shift_right<u1,u1>::value), ==, 0 );
    BOOST_MPL_ASSERT_RELATION( (shift_right<u2,u1>::value), ==, 1 );
    BOOST_MPL_ASSERT_RELATION( (shift_right<u8,u1>::value), ==, 4 );


See also
--------

|Bitwise Operations|, |Numeric Metafunction|, |numeric_cast|, |shift_left|, |bitand_|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
