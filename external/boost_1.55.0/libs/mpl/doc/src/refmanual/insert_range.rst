.. Sequences/Intrinsic Metafunctions//insert_range

insert_range
============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Pos
        , typename Range
        >
    struct insert_range
    {
        typedef |unspecified| type;
    };



Description
-----------

``insert_range`` performs an insertion of a range of elements at an arbitrary position in 
the sequence.

Header
------

.. parsed-literal::
    
    #include <boost/mpl/insert_range.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``Sequence``  | |Extensible Sequence| or          | A sequence to insert into.                    |
|               | |Extensible Associative Sequence| |                                               |
+---------------+-----------------------------------+-----------------------------------------------+
| ``Pos``       | |Forward Iterator|                | An iterator in ``Sequence`` specifying the    |
|               |                                   | insertion position.                           |
+---------------+-----------------------------------+-----------------------------------------------+
| ``Range``     | |Forward Sequence|                | The range of elements to be inserted.         |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Extensible Sequence| ``s``, iterator ``pos`` in ``s``, and |Forward Sequence| ``range``:

.. parsed-literal::

    typedef insert<s,pos,range>::type r; 

:Return type:
    |Extensible Sequence|.

:Precondition:
    ``pos`` is an iterator into ``s``. 

:Semantics:
    ``r`` is a sequence, |concept-identical| to ``s``, of the following elements: 
    [``begin<s>::type``, ``pos``), [``begin<r>::type``, ``end<r>::type``),
    [``pos``, ``end<s>::type``).

:Postcondition:
    The relative order of the elements in ``r`` is the same as in ``s``;

    .. parsed-literal::
    
        size<r>::value == size<s>::value + size<range>::value 


Complexity
----------

Sequence dependent. Quadratic in the worst case, linear at best; see the particular 
sequence class' specification for details.


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,0,1,7,8,9> numbers;
    typedef find< numbers,integral_c<int,7> >::type pos;
    typedef insert_range< numbers,pos,range_c<int,2,7> >::type range;

    BOOST_MPL_ASSERT_RELATION( size<range>::value, ==, 10 );
    BOOST_MPL_ASSERT(( equal< range,range_c<int,0,10> > ));

    typedef insert_range< 
          list\ ``0``\ <>
        , end< list\ ``0``\ <> >::type
        , list<int>
        >::type result2;
    
    BOOST_MPL_ASSERT_RELATION( size<result2>::value, ==, 1 );


See also
--------

|Extensible Sequence|, |insert|, |push_front|, |push_back|, |erase|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
