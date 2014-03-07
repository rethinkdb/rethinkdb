.. Algorithms/Querying Algorithms//count |40

count
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename T
        >
    struct count
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the number of elements in a ``Sequence`` that are identical to ``T``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/count.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------+
| Parameter     | Requirement               | Description                       |
+===============+===========================+===================================+
| ``Sequence``  | |Forward Sequence|        | A sequence to be examined.        |
+---------------+---------------------------+-----------------------------------+
| ``T``         | Any type                  | A type to search for.             |
+---------------+---------------------------+-----------------------------------+


Expression semantics
--------------------


For any |Forward Sequence| ``s`` and arbitrary type ``t``:


.. parsed-literal::

    typedef count<s,t>::type n;

:Return type:
    |Integral Constant|.
 
:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef count_if< s,is_same<_,T> >::type n;


Complexity
----------

Linear. Exactly ``size<s>::value`` comparisons for identity. 


Example
-------

.. parsed-literal::
    
    typedef vector<int,char,long,short,char,short,double,long> types;
    typedef count<types, short>::type n;
    
    BOOST_MPL_ASSERT_RELATION( n::value, ==, 2 );


See also
--------

|Querying Algorithms|, |count_if|, |find|, |find_if|, |contains|, |lower_bound|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
