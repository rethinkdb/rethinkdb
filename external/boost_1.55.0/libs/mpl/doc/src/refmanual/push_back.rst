.. Sequences/Intrinsic Metafunctions//push_back

push_back
=========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename T
        >
    struct push_back
    {
        typedef |unspecified| type;
    };



Description
-----------

``push_back`` performs an insertion at the end of the sequence with guaranteed |O(1)|
complexity.

Header
------

.. parsed-literal::
    
    #include <boost/mpl/push_back.hpp>



Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-------------------------------+-----------------------------------------------+
| Parameter     | Requirement                   | Description                                   |
+===============+===============================+===============================================+
| ``Sequence``  | |Back Extensible Sequence|    | A sequence to insert into.                    |
+---------------+-------------------------------+-----------------------------------------------+
| ``T``         | Any type                      | The element to be inserted.                   |
+---------------+-------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Back Extensible Sequence| ``s`` and arbitrary type ``x``:


.. parsed-literal::

    typedef push_back<s,x>::type r;

:Return type:
    |Back Extensible Sequence|.
    
:Semantics:
    Equivalent to 

    .. parsed-literal::
    
       typedef insert< s,end<s>::type,x >::type r;


:Postcondition:
    ``back<r>::type`` is identical to ``x``;

    .. parsed-literal::
    
       size<r>::value == size<s>::value + 1


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector_c<bool,false,false,false,
        true,true,true,false,false> bools;
    
    typedef push_back<bools,false_>::type message;
    
    BOOST_MPL_ASSERT_RELATION( back<message>::type::value, ==, false );
    BOOST_MPL_ASSERT_RELATION( 
          ( count_if<message, equal_to<_1,false_> >::value ), ==, 6
        );


See also
--------

|Back Extensible Sequence|, |insert|, |pop_back|, |back|, |push_front|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
