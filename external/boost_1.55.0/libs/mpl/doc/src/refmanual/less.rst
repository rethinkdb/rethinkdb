.. Metafunctions/Comparisons//less |10

less
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T1
        , typename T2
        >
    struct less
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a true-valued |Integral Constant| if ``T1`` is less than ``T2``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/less.hpp>
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

    typedef less<c1,c2>::type r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef bool_< (c1::value < c2::value) > r;


.. ..........................................................................

.. parsed-literal::

    typedef less<c1,c2> r; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        struct r : less<c1,c2>::type {};



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    BOOST_MPL_ASSERT(( less< int_<0>, int_<10> > ));
    BOOST_MPL_ASSERT_NOT(( less< long_<10>, int_<0> > ));
    BOOST_MPL_ASSERT_NOT(( less< long_<10>, int_<10> > ));


See also
--------

|Comparisons|, |Numeric Metafunction|, |numeric_cast|, |less_equal|, |greater|, |equal|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
