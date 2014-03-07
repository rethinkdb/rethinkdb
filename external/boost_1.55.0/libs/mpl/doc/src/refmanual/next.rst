.. Iterators/Iterator Metafunctions//next |30

next
====

Synopsis
--------

.. parsed-literal::

    template<
          typename Iterator
        >
    struct next
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the next iterator in the sequence. |Note:| ``next`` has a number of 
overloaded meanings, depending on the type of its argument. For instance,
if ``X`` is an |Integral Constant|, ``next<X>`` returns an incremented 
|Integral Constant| of the same type. The following specification is 
iterator-specific. Please refer to the corresponding concept's
documentation for the details of the alternative semantics |-- end note|. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/next_prior.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------+
| Parameter     | Requirement               | Description                       |
+===============+===========================+===================================+
| ``Iterator``  | |Forward Iterator|.       | An iterator to increment.         |
+---------------+---------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Iterator|\ s ``iter``:


.. parsed-literal::

    typedef next<iter>::type j; 

:Return type:
    |Forward Iterator|.

:Precondition:
    ``iter`` is incrementable.

:Semantics:
    ``j`` is an iterator pointing to the next element in the sequence, or 
    is past-the-end. If ``iter`` is a user-defined iterator, the 
    library-provided default implementation is equivalent to

    .. parsed-literal::
    
        typedef iter::next j;


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,1> v;
    typedef begin<v>::type first;
    typedef end<v>::type last;
    
    BOOST_MPL_ASSERT(( is_same< next<first>::type, last > ));


See also
--------

|Iterators|, |begin| / |end|, |prior|, |deref|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
