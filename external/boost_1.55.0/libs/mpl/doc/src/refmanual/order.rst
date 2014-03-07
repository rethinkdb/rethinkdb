.. Sequences/Intrinsic Metafunctions//order

order
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Key
        >
    struct order
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a unique unsigned |Integral Constant| associated with the key ``Key`` in 
``Sequence``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/order.hpp>


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

    typedef order<s,key>::type n; 

:Return type:
    Unsigned |Integral Constant|.
    
:Semantics:
    If ``has_key<s,key>::value == true``, ``n`` is a unique unsigned 
    |Integral Constant| associated with ``key`` in ``s``; otherwise, 
    ``n`` is identical to ``void_``.


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef map< pair<int,unsigned>, pair<char,long> > m;

    BOOST_MPL_ASSERT_NOT(( is_same< order<m,int>::type, void\_ > ));
    BOOST_MPL_ASSERT(( is_same< order<m,long>::type,void\_ > ));



See also
--------

|Associative Sequence|, |has_key|, |count|, |map|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
