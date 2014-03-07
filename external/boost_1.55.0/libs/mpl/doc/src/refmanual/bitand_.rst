.. Metafunctions/Bitwise Operations//bitand_

bitand\_
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
    struct bitand\_
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of *bitwise and* (``&``) operation of its arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/bitand.hpp>
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

    typedef bitand_<c1,\ |...|\ c\ *n*\>::type r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 
        
    .. parsed-literal::
    
        typedef integral_c<
              typeof(c1::value & c2::value)
            , ( c1::value & c2::value )
            > c;
            
        typedef bitand_<c,c3,\ |...|\c\ *n*\>::type r; 

.. ..........................................................................

.. parsed-literal::

    typedef bitand_<c1,\ |...|\ c\ *n*\> r;

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to
    
    .. parsed-literal::

        struct r : bitand_<c1,\ |...|\ c\ *n*\>::type {};


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
    
    BOOST_MPL_ASSERT_RELATION( (bitand_<u0,u0>::value), ==, 0 );
    BOOST_MPL_ASSERT_RELATION( (bitand_<u1,u0>::value), ==, 0 );
    BOOST_MPL_ASSERT_RELATION( (bitand_<u0,u1>::value), ==, 0 );
    BOOST_MPL_ASSERT_RELATION( (bitand_<u0,uffffffff>::value), ==, 0 );
    BOOST_MPL_ASSERT_RELATION( (bitand_<u1,uffffffff>::value), ==, 1 );
    BOOST_MPL_ASSERT_RELATION( (bitand_<u8,uffffffff>::value), ==, 8 );


See also
--------

|Bitwise Operations|, |Numeric Metafunction|, |numeric_cast|, |bitor_|, |bitxor_|, |shift_left|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
