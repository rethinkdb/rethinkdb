.. Sequences/Intrinsic Metafunctions//pop_back

pop_back
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct pop_back
    {
        typedef |unspecified| type;
    };


Description
-----------

``pop_back`` performs a removal at the end of the sequence with guaranteed |O(1)|
complexity.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/pop_back.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-------------------------------+-----------------------------------------------+
| Parameter     | Requirement                   | Description                                   |
+===============+===============================+===============================================+
| ``Sequence``  | |Back Extensible Sequence|    | A sequence to erase the last element from.    |
+---------------+-------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Back Extensible Sequence| ``s``:

.. parsed-literal::

    typedef pop_back<s>::type r; 

:Return type:
    |Back Extensible Sequence|.

:Precondition:
    ``empty<s>::value == false``.
 
:Semantics:
    Equivalent to ``erase<s,end<s>::type>::type;``.

:Postcondition:
    ``size<r>::value == size<s>::value - 1``.


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector<long>::type types1;
    typedef vector<long,int>::type types2;
    typedef vector<long,int,char>::type types3;
    
    typedef pop_back<types1>::type result1;
    typedef pop_back<types2>::type result2;
    typedef pop_back<types3>::type result3;
        
    BOOST_MPL_ASSERT_RELATION( size<result1>::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( size<result2>::value, ==, 1 );
    BOOST_MPL_ASSERT_RELATION( size<result3>::value, ==, 2 );
        
    BOOST_MPL_ASSERT(( is_same< back<result2>::type, long> ));
    BOOST_MPL_ASSERT(( is_same< back<result3>::type, int > ));


See also
--------

|Back Extensible Sequence|, |erase|, |push_back|, |back|, |pop_front|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
