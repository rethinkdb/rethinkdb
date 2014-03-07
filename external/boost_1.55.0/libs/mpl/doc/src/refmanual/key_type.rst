.. Sequences/Intrinsic Metafunctions//key_type

key_type
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename X
        >
    struct key_type
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the |key| that would be used to identify ``X`` in ``Sequence``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/key_type.hpp>


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
| ``X``         | Any type                  | The type to get the |key| for.                |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Associative Sequence| ``s``, iterators ``pos1`` and ``pos2`` in ``s``, and an 
artibrary type ``x``:

.. parsed-literal::

    typedef key_type<s,x>::type k; 

:Return type:
    A type.
    
:Precondition:
    ``x`` can be put in ``s``. 

:Semantics:
    ``k`` is the |key| that would be used to identify ``x`` in ``s``.

:Postcondition:
    If ``key_type< s,deref<pos1>::type >::type`` is identical to
    ``key_type< s,deref<pos2>::type >::type`` then ``pos1`` is identical to ``pos2``.



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef key_type< map<>,pair<int,unsigned> >::type k1;
    typedef key_type< set<>,pair<int,unsigned> >::type k2;

    BOOST_MPL_ASSERT(( is_same< k1,int > ));
    BOOST_MPL_ASSERT(( is_same< k2,pair<int,unsigned> > ));


See also
--------

|Associative Sequence|, |value_type|, |has_key|, |set|, |map|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
