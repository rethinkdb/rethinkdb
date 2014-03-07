.. Metafunctions/Comparisons//not_equal_to |60

not_equal_to
============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T1
        , typename T2
        >
    struct not_equal_to
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a true-valued |Integral Constant| if ``T1`` and ``T2`` are not equal.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/not_equal_to.hpp>
    #include <boost/mpl/comparison.hpp>


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

    typedef not_equal_to<c1,c2>::type r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef bool_< (c1::value != c2::value) > r;


.. ..........................................................................

.. parsed-literal::

    typedef not_equal_to<c1,c2> r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        struct r : not_equal_to<c1,c2>::type {};



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    BOOST_MPL_ASSERT(( not_equal_to< int_<0>, int_<10> > ));
    BOOST_MPL_ASSERT(( not_equal_to< long_<10>, int_<0> > ));
    BOOST_MPL_ASSERT_NOT(( not_equal_to< long_<10>, int_<10> > ));


See also
--------

|Comparisons|, |Numeric Metafunction|, |numeric_cast|, |equal_to|, |less|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
