.. Sequences/Intrinsic Metafunctions//at_c

at_c
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , long n
        >
    struct at_c
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a type identical to the ``n``\ th element from the beginning of 
the sequence. ``at_c<Sequence,n>::type`` is a shorcut notation for 
``at< Sequence, long_<n> >::type``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/at.hpp>



Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``Sequence``  | |Forward Sequence|                | A sequence to be examined.                    |
+---------------+-----------------------------------+-----------------------------------------------+
| ``n``         | A compile-time integral constant  | An offset from the beginning of the sequence  |
|               |                                   | specifying the element to be retrieved.       |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------


.. parsed-literal::

    typedef at_c<Sequence,n>::type t; 

:Return type:
    A type 

:Precondition:
    ``0 <= n < size<Sequence>::value`` 

:Semantics:
    Equivalent to 
    
    .. parsed-literal::

       typedef at< Sequence, long_<n> >::type t;


Complexity
----------

+-------------------------------+-----------------------------------+
| Sequence archetype            | Complexity                        |
+===============================+===================================+
| |Forward Sequence|            | Linear.                           |
+-------------------------------+-----------------------------------+
| |Random Access Sequence|      | Amortized constant time.          |
+-------------------------------+-----------------------------------+


Example
-------

.. parsed-literal::
    
    typedef range_c<long,10,50> range;
    BOOST_MPL_ASSERT_RELATION( (at_c< range,0 >::type::value), ==, 10 );
    BOOST_MPL_ASSERT_RELATION( (at_c< range,10 >::type::value), ==, 20 );
    BOOST_MPL_ASSERT_RELATION( (at_c< range,40 >::type::value), ==, 50 );


See also
--------

|Forward Sequence|, |Random Access Sequence|, |at|, |front|, |back|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
