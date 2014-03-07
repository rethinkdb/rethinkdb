.. Sequences/Intrinsic Metafunctions//back

back
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct back
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the last element in the sequence.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/back.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+---------------------------+-----------------------------------+
| Parameter     | Requirement               | Description                       |
+===============+===========================+===================================+
| ``Sequence``  | |Bidirectional Sequence|  | A sequence to be examined.        |
+---------------+---------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Bidirectional Sequence| ``s``:

.. parsed-literal::

    typedef back<s>::type t; 

:Return type:
    A type.

:Precondition:
    ``empty<s>::value == false``.

:Semantics:
    Equivalent to

    .. parsed-literal::
    
       typedef deref< prior< end<s>::type >::type >::type t;



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef range_c<int,0,1> range1;
    typedef range_c<int,0,10> range2;
    typedef range_c<int,-10,0> range3;
        
    BOOST_MPL_ASSERT_RELATION( back<range1>::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( back<range2>::value, ==, 9 );
    BOOST_MPL_ASSERT_RELATION( back<range3>::value, ==, -1 );


See also
--------

|Bidirectional Sequence|, |front|, |push_back|, |end|, |deref|, |at|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
