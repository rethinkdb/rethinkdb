.. Sequences/Intrinsic Metafunctions//front

front
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct front
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the first element in the sequence.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/front.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------+-----------------------------------------------+
| Parameter     | Requirement           | Description                                   |
+===============+=======================+===============================================+
| ``Sequence``  | |Forward Sequence|    | A sequence to be examined.                    |
+---------------+-----------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s``:


.. parsed-literal::

    typedef front<s>::type t; 

:Return type:
    A type.

:Precondition:
    ``empty<s>::value == false``.

:Semantics:
    Equivalent to 

    .. parsed-literal::
        
       typedef deref< begin<s>::type >::type t;



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef list<long>::type types1;
    typedef list<int,long>::type types2;
    typedef list<char,int,long>::type types3;
    
    BOOST_MPL_ASSERT(( is_same< front<types1>::type, long > ));
    BOOST_MPL_ASSERT(( is_same< front<types2>::type, int> ));
    BOOST_MPL_ASSERT(( is_same< front<types3>::type, char> ));



See also
--------

|Forward Sequence|, |back|, |push_front|, |begin|, |deref|, |at|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
