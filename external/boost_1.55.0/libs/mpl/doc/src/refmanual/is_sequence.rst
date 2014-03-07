.. Sequences/Intrinsic Metafunctions//is_sequence

is_sequence
===========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename X
        >
    struct is_sequence
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a boolean |Integral Constant| ``c`` such that ``c::value == true`` if and 
only if ``X`` is a model of |Forward Sequence|.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/is_sequence.hpp>


Parameters
----------

+---------------+-------------------+-----------------------------------------------+
| Parameter     | Requirement       | Description                                   |
+===============+===================+===============================================+
| ``X``         | Any type          | The type to query.                            |
+---------------+-------------------+-----------------------------------------------+


Expression semantics
--------------------


.. parsed-literal::

    typedef is_sequence<X>::type c; 

:Return type:
    Boolean |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef not_< is_same< begin<T>::type,void_ > >::type c;



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    struct UDT {};

    BOOST_MPL_ASSERT_NOT(( is_sequence< std::vector<int> > ));
    BOOST_MPL_ASSERT_NOT(( is_sequence< int > ));
    BOOST_MPL_ASSERT_NOT(( is_sequence< int& > ));
    BOOST_MPL_ASSERT_NOT(( is_sequence< UDT > ));
    BOOST_MPL_ASSERT_NOT(( is_sequence< UDT* > ));
    BOOST_MPL_ASSERT(( is_sequence< range_c<int,0,0> > ));
    BOOST_MPL_ASSERT(( is_sequence< list<> > ));
    BOOST_MPL_ASSERT(( is_sequence< list<int> > ));
    BOOST_MPL_ASSERT(( is_sequence< vector<> > ));
    BOOST_MPL_ASSERT(( is_sequence< vector<int> > ));


See also
--------

|Forward Sequence|, |begin|, |end|, |vector|, |list|, |range_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
