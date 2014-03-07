.. Iterators/Iterator Metafunctions//advance |10

advance
=======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Iterator
        , typename N
        >
    struct advance
    {
        typedef |unspecified| type;
    };



Description
-----------

Moves ``Iterator`` by the distance ``N``. For |bidirectional| and 
|random access| iterators, the distance may be negative. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/advance.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------+
| Parameter     | Requirement               | Description                       |
+===============+===========================+===================================+
| ``Iterator``  | |Forward Iterator|        | An iterator to advance.           |
+---------------+---------------------------+-----------------------------------+
| ``N``         | |Integral Constant|       | A distance.                       |
+---------------+---------------------------+-----------------------------------+


Model Of
--------

|Tag Dispatched Metafunction|


Expression semantics
--------------------

For a |Forward Iterator| ``iter`` and arbitrary |Integral Constant| ``n``:

.. parsed-literal::

    typedef advance<iter,n>::type j; 

:Return type:
    |Forward Iterator|.

:Precondition:
    If ``Iterator`` is a |Forward Iterator|, ``n::value`` must be nonnegative.

:Semantics:
    Equivalent to: 
    
    .. parsed-literal::
    
        typedef iter i0; 
        typedef next<i0>::type i1; 
        |...|
        typedef next<i\ *n-1*\ >::type j;
        
    if ``n::value > 0``, and 
    
    .. parsed-literal::

        typedef iter i0; 
        typedef prior<i0>::type i1; 
        |...|
        typedef prior<i\ *n-1*\ >::type j;
    
    otherwise.


:Postcondition:
    ``j`` is dereferenceable or past-the-end;
    ``distance<iter,j>::value == n::value`` if ``n::value > 0``, and 
    ``distance<j,iter>::value == n::value`` otherwise.


Complexity
----------

Amortized constant time if ``iter`` is a model of 
|Random Access Iterator|, otherwise linear time. 


Example
-------

.. parsed-literal::
    
    typedef range_c<int,0,10> numbers;
    typedef begin<numbers>::type first;
    typedef end<numbers>::type last;

    typedef advance<first,int_<10> >::type i1;
    typedef advance<last,int_<-10> >::type i2;
    
    BOOST_MPL_ASSERT(( boost::is_same<i1,last> ));
    BOOST_MPL_ASSERT(( boost::is_same<i2,first> ));


See also
--------

|Iterators|, |Tag Dispatched Metafunction|, |distance|, |next|

.. |bidirectional| replace:: `bidirectional`_
.. _bidirectional: `Bidirectional Iterator`_
.. |random access| replace:: `random access`_
.. _random access: `Random Access Iterator`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
