.. Metafunctions/Bitwise Operations//bitxor_

bitxor\_
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T1
        , typename T2
        , typename T3 = |unspecified|
        |...|
        , typename T\ *n* = |unspecified|
        >
    struct bitxor\_
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of *bitwise xor* (``^``) operation of its arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/bitxor.hpp>
    #include <boost/mpl/bitwise.hpp>


Model of
--------

|Numeric Metafunction|


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| |T1...Tn|     | |Integral Constant|       | Operation's arguments.                        |
+---------------+---------------------------+-----------------------------------------------+

|Note:| |numeric metafunction note| |-- end note|


Expression semantics
--------------------

For any |Integral Constant|\ s |c1...cn|:


.. parsed-literal::

    typedef bitxor_<c1,\ |...|\ c\ *n*\>::type r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 
        
    .. parsed-literal::
    
        typedef integral_c<
              typeof(c1::value ^ c2::value)
            , ( c1::value ^ c2::value )
            > c;
            
        typedef bitxor_<c,c3,\ |...|\c\ *n*\>::type r; 

.. ..........................................................................

.. parsed-literal::

    typedef bitxor_<c1,\ |...|\ c\ *n*\> r;

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to
    
    .. parsed-literal::

        struct r : bitxor_<c1,\ |...|\ c\ *n*\>::type {};


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
    typedef integral_c<unsigned,0xffffffff> uffffffff;
    
    BOOST_MPL_ASSERT_RELATION( (bitxor_<u0,u0>::value), ==, 0 );
    BOOST_MPL_ASSERT_RELATION( (bitxor_<u1,u0>::value), ==, 1 );
    BOOST_MPL_ASSERT_RELATION( (bitxor_<u0,u1>::value), ==, 1 );
    
    BOOST_MPL_ASSERT_RELATION( (bitxor_<u0,uffffffff>::value), ==, 0xffffffff ^ 0 );
    BOOST_MPL_ASSERT_RELATION( (bitxor_<u1,uffffffff>::value), ==, 0xffffffff ^ 1 );
    BOOST_MPL_ASSERT_RELATION( (bitxor_<u8,uffffffff>::value), ==, 0xffffffff ^ 8 );


See also
--------

|Bitwise Operations|, |Numeric Metafunction|, |numeric_cast|, |bitand_|, |bitor_|, |shift_left|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
