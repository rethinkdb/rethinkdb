.. Sequences/Intrinsic Metafunctions//size

size
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct size
    {
        typedef |unspecified| type;
    };



Description
-----------

``size`` returns the number of elements in the sequence, that is, the number of elements 
in the range [``begin<Sequence>::type``, ``end<Sequence>::type``).


Header
------

.. parsed-literal::
    
    #include <boost/mpl/size.hpp>



Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------+-----------------------------------------------+
| Parameter     | Requirement           | Description                                   |
+===============+=======================+===============================================+
| ``Sequence``  | |Forward Sequence|    | A sequence to query.                          |
+---------------+-----------------------+-----------------------------------------------+


Expression semantics
--------------------


For any |Forward Sequence| ``s``:


.. parsed-literal::

    typedef size<s>::type n; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
       typedef distance< begin<s>::type,end<s>::type >::type n;


:Postcondition:
    ``n::value >= 0``.



Complexity
----------

The complexity of the ``size`` metafunction directly depends on the implementation of 
the particular sequence it is applied to. In the worst case, ``size`` guarantees a 
linear complexity.

If the ``s`` is a |Random Access Sequence|, ``size<s>::type`` is an |O(1)| operation. 
The opposite is not necessarily true |--| for example, a sequence class that models 
|Forward Sequence| might still give us an |O(1)| ``size`` implementation.


Example
-------

.. parsed-literal::
    
    typedef list0<> empty_list;
    typedef vector_c<int,0,1,2,3,4,5> numbers;
    typedef range_c<int,0,100> more_numbers;
    
    BOOST_MPL_ASSERT_RELATION( size<list>::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( size<numbers>::value, ==, 5 );
    BOOST_MPL_ASSERT_RELATION( size<more_numbers>::value, ==, 100 );


See also
--------

|Forward Sequence|, |Random Access Sequence|, |empty|, |begin|, |end|, |distance|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
