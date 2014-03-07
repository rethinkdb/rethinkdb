.. Metafunctions/Arithmetic Operations//divides |40

divides
=======

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
    struct divides
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the quotient of its arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/divides.hpp>
    #include <boost/mpl/arithmetic.hpp>


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

    typedef divides<c1,\ |...|\ c\ *n*\>::type r; 

:Return type:
    |Integral Constant|.

:Precondition:
    ``c2::value != 0``, |...| ``cn::value != 0``.
 
:Semantics:
    Equivalent to 
    
    .. parsed-literal::
    
        typedef integral_c<
              typeof(c1::value / c2::value)
            , ( c1::value / c2::value )
            > c;
            
        typedef divides<c,c3,\ |...|\c\ *n*\>::type r; 

.. ..........................................................................

.. parsed-literal::

    typedef divides<c1,\ |...|\ c\ *n*\> r;

:Return type:
    |Integral Constant|.

:Precondition:
    ``c2::value != 0``, |...| ``cn::value != 0``.
 
:Semantics:
    Equivalent to 
    
    .. parsed-literal::
    
        struct r : divides<c1,\ |...|\ c\ *n*\>::type {};


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef divides< int_<-10>, int_<3>, long_<1> >::type r;
    BOOST_MPL_ASSERT_RELATION( r::value, ==, -3 );
    BOOST_MPL_ASSERT(( is_same< r::value_type, long > ));


See also
--------

|Arithmetic Operations|, |Numeric Metafunction|, |numeric_cast|, |times|, |modulus|, |plus|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
