.. Sequences/Intrinsic Metafunctions//begin

begin
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename X
        >
    struct begin
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns an iterator that points to the first element of the sequence. If
the argument is not a |Forward Sequence|, returns |void_|.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/begin_end.hpp>



Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-------------------+---------------------------------------------------+
| Parameter     | Requirement       | Description                                       |
+===============+===================+===================================================+
| ``X``         | Any type          | A type whose begin iterator, if any, will be      |
|               |                   | returned.                                         |
+---------------+-------------------+---------------------------------------------------+


Expression semantics
--------------------

For any arbitrary type ``x``:

.. parsed-literal::

    typedef begin<x>::type first;

:Return type:
    |Forward Iterator| or |void_|.

:Semantics:
    If ``x`` is a |Forward Sequence|, ``first`` is an iterator pointing to the 
    first element of ``s``; otherwise ``first`` is |void_|.
    
:Postcondition:
    If ``first`` is an iterator, it is either dereferenceable or past-the-end; it 
    is past-the-end if and only if ``size<x>::value == 0``. 


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector< unsigned char,unsigned short,
        unsigned int,unsigned long > unsigned_types;
    
    typedef begin<unsigned_types>::type iter;
    BOOST_MPL_ASSERT(( is_same< deref<iter>::type, unsigned char > ));
 
    BOOST_MPL_ASSERT(( is_same< begin<int>::type, void\_ > ));


See also
--------

|Iterators|, |Forward Sequence|, |end|, |size|, |empty|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
