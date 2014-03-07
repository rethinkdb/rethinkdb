.. Sequences/Intrinsic Metafunctions//has_key

has_key
=======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Key
        >
    struct has_key
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a true-valued |Integral Constant| if ``Sequence`` contains an element
with key ``Key``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/has_key.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| ``Sequence``  | |Associative Sequence|    | A sequence to query.                          |
+---------------+---------------------------+-----------------------------------------------+
| ``Key``       | Any type                  | The queried key.                              |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Associative Sequence| ``s``, and arbitrary type ``key``:

.. parsed-literal::

    typedef has_key<s,key>::type c; 

:Return type:
    Boolean |Integral Constant|.
    
:Semantics:
    ``c::value == true`` if ``key`` is in ``s``'s set of keys; otherwise
    ``c::value == false``.


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef map< pair<int,unsigned>, pair<char,long> > m;
    BOOST_MPL_ASSERT_NOT(( has_key<m,long> ));

    typedef insert< m, pair<long,unsigned long> > m1;
    BOOST_MPL_ASSERT(( has_key<m1,long> ));


See also
--------

|Associative Sequence|, |count|, |insert|, |erase_key|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
