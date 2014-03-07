.. Sequences/Intrinsic Metafunctions//value_type

value_type
==========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename X
        >
    struct value_type
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the |value| that would be used for element ``X`` in ``Sequence``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/value_type.hpp>


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
| ``X``         | Any type                  | The type to get the |value| for.              |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Associative Sequence| ``s``, and an artibrary type ``x``:


.. parsed-literal::

    typedef value_type<s,x>::type v; 

:Return type:
    A type.
    
:Precondition:
    ``x`` can be put in ``s``.

:Semantics:
    ``v`` is the |value| that would be used for ``x`` in ``s``.

:Postcondition:
    If     
    .. parsed-literal::
       
       has_key< s,key_type<s,x>::type >::type
    
    then
    .. parsed-literal::
    
       at< s,key_type<s,x>::type >::type
    
    is identical to ``value_type<s,x>::type``.



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef value_type< map<>,pair<int,unsigned> >::type v1;
    typedef value_type< set<>,pair<int,unsigned> >::type v2;

    BOOST_MPL_ASSERT(( is_same< v1,unsigned > ));
    BOOST_MPL_ASSERT(( is_same< v2,pair<int,unsigned> > ));


See also
--------

|Associative Sequence|, |key_type|, |at|, |set|, |map|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
