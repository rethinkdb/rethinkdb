.. Sequences/Intrinsic Metafunctions//pop_front

pop_front
=========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct pop_front
    {
        typedef |unspecified| type;
    };



Description
-----------

``pop_front`` performs a removal at the beginning of the sequence with guaranteed |O(1)|
complexity.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/pop_front.hpp>



Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``Sequence``  | |Front Extensible Sequence|       | A sequence to erase the first element from.   |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Front Extensible Sequence| ``s``:

.. parsed-literal::

    typedef pop_front<s>::type r; 

:Return type:
    |Front Extensible Sequence|.

:Precondition:
    ``empty<s>::value == false``.

:Semantics:
    Equivalent to ``erase<s,begin<s>::type>::type;``.

:Postcondition:
    ``size<r>::value == size<s>::value - 1``.


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector<long>::type types1;
    typedef vector<int,long>::type types2;
    typedef vector<char,int,long>::type types3;
    
    typedef pop_front<types1>::type result1;
    typedef pop_front<types2>::type result2;
    typedef pop_front<types3>::type result3;
        
    BOOST_MPL_ASSERT_RELATION( size<result1>::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( size<result2>::value, ==, 1 );
    BOOST_MPL_ASSERT_RELATION( size<result3>::value, ==, 2 );
        
    BOOST_MPL_ASSERT(( is_same< front<result2>::type, long > ));
    BOOST_MPL_ASSERT(( is_same< front<result3>::type, int > ));


See also
--------

|Front Extensible Sequence|, |erase|, |push_front|, |front|, |pop_back|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
