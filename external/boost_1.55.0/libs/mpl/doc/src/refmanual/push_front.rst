.. Sequences/Intrinsic Metafunctions//push_front

push_front
==========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename T
        >
    struct push_front
    {
        typedef |unspecified| type;
    };



Description
-----------

``push_front`` performs an insertion at the beginning of the sequence with guaranteed |O(1)|
complexity.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/push_front.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-------------------------------+-----------------------------------------------+
| Parameter     | Requirement                   | Description                                   |
+===============+===============================+===============================================+
| ``Sequence``  | |Front Extensible Sequence|   | A sequence to insert into.                    |
+---------------+-------------------------------+-----------------------------------------------+
| ``T``         | Any type                      | The element to be inserted.                   |
+---------------+-------------------------------+-----------------------------------------------+



Expression semantics
--------------------


For any |Front Extensible Sequence| ``s`` and arbitrary type ``x``:


.. parsed-literal::

    typedef push_front<s,x>::type r;

:Return type:
    |Front Extensible Sequence|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
       typedef insert< s,begin<s>::type,x >::type r;


:Postcondition:
    ``size<r>::value == size<s>::value + 1``; 
    ``front<r>::type`` is identical to ``x``.


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,1,2,3,5,8,13,21> v;
    BOOST_MPL_ASSERT_RELATION( size<v>::value, ==, 7 );
    
    typedef push_front< v,integral_c<int,1> >::type fibonacci;
    BOOST_MPL_ASSERT_RELATION( size<fibonacci>::value, ==, 8 );
    
    BOOST_MPL_ASSERT(( equal< 
          fibonacci
        , vector_c<int,1,1,2,3,5,8,13,21>
        , equal_to<_,_>
        > ));


See also
--------

|Front Extensible Sequence|, |insert|, |pop_front|, |front|, |push_back|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
