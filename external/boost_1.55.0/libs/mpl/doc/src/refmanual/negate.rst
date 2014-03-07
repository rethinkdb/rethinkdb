.. Metafunctions/Arithmetic Operations//negate |60

negate
======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T
        >
    struct negate
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the negative (additive inverse) of its argument.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/negate.hpp>
    #include <boost/mpl/arithmetic.hpp>


Model of
--------

|Numeric Metafunction|


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| ``T``         | |Integral Constant|       | Operation's argument.                         |
+---------------+---------------------------+-----------------------------------------------+

|Note:| |numeric metafunction note| |-- end note|


Expression semantics
--------------------

For any |Integral Constant| ``c``:

.. parsed-literal::

    typedef negate<c>::type r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef integral_c< c::value_type, ( -c::value ) > r;

.. ..........................................................................

.. parsed-literal::

    typedef negate<c> r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to
    
    .. parsed-literal::

        struct r : negate<c>::type {};


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef negate< int_<-10> >::type r;
    BOOST_MPL_ASSERT_RELATION( r::value, ==, 10 );
    BOOST_MPL_ASSERT(( is_same< r::value_type, int > ));


See also
--------

|Arithmetic Operations|, |Numeric Metafunction|, |numeric_cast|, |plus|, |minus|, |times|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
