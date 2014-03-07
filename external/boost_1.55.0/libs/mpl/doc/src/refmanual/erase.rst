.. Sequences/Intrinsic Metafunctions//erase

erase
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename First
        , typename Last = |unspecified|
        >
    struct erase
    {
        typedef |unspecified| type;
    };



Description
-----------

``erase`` performs a removal of one or more adjacent elements in the sequence 
starting from an arbitrary position.

Header
------

.. parsed-literal::
    
    #include <boost/mpl/erase.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``Sequence``  | |Extensible Sequence| or          | A sequence to erase from.                     |
|               | |Extensible Associative Sequence| |                                               |
+---------------+-----------------------------------+-----------------------------------------------+
| ``First``     | |Forward Iterator|                | An iterator to the beginning of the range to  |
|               |                                   | be erased.                                    |
+---------------+-----------------------------------+-----------------------------------------------+
| ``Last``      | |Forward Iterator|                | An iterator past-the-end of the range to be   |
|               |                                   | erased.                                       |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

.. compound::
    :class: expression-semantics

    For any |Extensible Sequence| ``s``, and iterators ``pos``, ``first`` and ``last`` into ``s``:


    .. parsed-literal::

        typedef erase<s,first,last>::type r; 

    :Return type:
        |Extensible Sequence|.

    :Precondition:
        ``[first,last)`` is a valid range in ``s``. 
     
    :Semantics:
        ``r`` is a new sequence, |concept-identical| to ``s``, of the following elements:
        [``begin<s>::type``, ``pos``), [``last``, ``end<s>::type``).

    :Postcondition:
        The relative order of the elements in ``r`` is the same as in ``s``;

        .. parsed-literal::

           size<r>::value == size<s>::value - distance<first,last>::value


    .. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    .. parsed-literal::

        typedef erase<s,pos>::type r;

    :Return type:
        |Extensible Sequence|.
        
    :Precondition:
        ``pos`` is a dereferenceable iterator in ``s``.

    :Semantics:
        Equivalent to 
        
        .. parsed-literal::

           typedef erase< s,pos,next<pos>::type >::type r;



.. compound::
    :class: expression-semantics

    For any |Extensible Associative Sequence| ``s``, and iterator ``pos`` into ``s``:

    .. parsed-literal::

        typedef erase<s,pos>::type r;

    :Return type:
        |Extensible Sequence|.
        
    :Precondition:
        ``pos`` is a dereferenceable iterator to ``s``.

    :Semantics:
        Erases the element at a specific position ``pos``; equivalent to
        ``erase_key<s, deref<pos>::type >::type``.

    :Postcondition:
        ``size<r>::value == size<s>::value - 1``. 


Complexity
----------

+---------------------------------------+-----------------------------------------------+
| Sequence archetype                    | Complexity (the range form)                   |
+=======================================+===============================================+
| |Extensible Associative Sequence|     | Amortized constant time.                      |
+---------------------------------------+-----------------------------------------------+
| |Extensible Sequence|                 | Quadratic in the worst case, linear at best.  |
+---------------------------------------+-----------------------------------------------+


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,1,0,5,1,7,5,0,5> values;
    typedef find< values, integral_c<int,7> >::type pos;
    typedef erase<values,pos>::type result;
    
    BOOST_MPL_ASSERT_RELATION( size<result>::value, ==, 7 );
    
    typedef find<result, integral_c<int,7> >::type iter;
    BOOST_MPL_ASSERT(( is_same< iter, end<result>::type > ));


See also
--------

|Extensible Sequence|, |Extensible Associative Sequence|, |erase_key|, |pop_front|, |pop_back|, |insert|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
