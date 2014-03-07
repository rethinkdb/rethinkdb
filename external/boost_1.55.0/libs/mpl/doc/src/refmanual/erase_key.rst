.. Sequences/Intrinsic Metafunctions//erase_key

erase_key
=========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename AssocSeq
        , typename Key
        >
    struct erase_key
    {
        typedef |unspecified| type;
    };



Description
-----------

Erases elements associated with the key ``Key`` in the |Extensible Associative Sequence| 
``AssocSeq`` . 

Header
------

.. parsed-literal::
    
    #include <boost/mpl/erase_key.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``AssocSeq``  | |Extensible Associative Sequence| | A sequence to erase elements from.            |
+---------------+-----------------------------------+-----------------------------------------------+
| ``Key``       | Any type                          | A key for the elements to be removed.         |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Extensible Associative Sequence| ``s``, and arbitrary type ``key``:


.. parsed-literal::

    typedef erase_key<s,key>::type r; 

:Return type:
    |Extensible Associative Sequence|.
    
:Semantics:
    ``r`` is |concept-identical| and equivalent to ``s`` except that 
    ``has_key<r,k>::value == false``.

:Postcondition:
    ``size<r>::value == size<s>::value - 1``. 



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef map< pair<int,unsigned>, pair<char,long> > m;
    typedef erase_key<m,char>::type m1;

    BOOST_MPL_ASSERT_RELATION( size<m1>::type::value, ==, 1 );
    BOOST_MPL_ASSERT(( is_same< at<m1,char>::type,void\_ > ));
    BOOST_MPL_ASSERT(( is_same< at<m1,int>::type,unsigned > ));


See also
--------

|Extensible Associative Sequence|, |erase|, |has_key|, |insert|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
