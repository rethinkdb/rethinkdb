.. Iterators/Iterator Metafunctions//distance |20

distance
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename First
        , typename Last
        >
    struct distance
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the distance between ``First`` and ``Last`` iterators, that is, an 
|Integral Constant| ``n`` such that ``advance<First,n>::type`` is 
identical to ``Last``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/distance.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------+
| Parameter     | Requirement               | Description                       |
+===============+===========================+===================================+
| ``First``,    | |Forward Iterator|        | Iterators to compute a            |
| ``Last``      |                           | distance between.                 |
+---------------+---------------------------+-----------------------------------+

Model Of
--------

|Tag Dispatched Metafunction|


Expression semantics
--------------------

For any |Forward Iterator|\ s ``first`` and ``last``:

.. parsed-literal::

    typedef distance<first,last>::type n; 

:Return type:
    |Integral Constant|.

:Precondition:
    [``first``, ``last``) is a valid range.

:Semantics:
    Equivalent to
    
    .. parsed-literal::
    
        typedef iter_fold<
              iterator_range<first,last>
            , long_<0>
            , next<_1>
            >::type n;
        

:Postcondition:
    ``is_same< advance<first,n>::type, last >::value == true``.


Complexity
----------

Amortized constant time if ``first`` and ``last`` are |Random Access Iterator|\ s,
otherwise linear time.


Example
-------

.. parsed-literal::
    
    typedef range_c<int,0,10>::type range;
    typedef begin<range>::type first;
    typedef end<range>::type last;
    
    BOOST_MPL_ASSERT_RELATION( (distance<first,last>::value), ==, 10);


See also
--------

|Iterators|, |Tag Dispatched Metafunction|, |advance|, |next|, |prior|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
