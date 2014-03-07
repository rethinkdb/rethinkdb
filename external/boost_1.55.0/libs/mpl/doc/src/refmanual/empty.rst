.. Sequences/Intrinsic Metafunctions//empty

empty
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct empty
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns an |Integral Constant| ``c`` such that ``c::value == true`` if 
and only if the sequence is empty.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/empty.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------+-----------------------------------+
| Parameter     | Requirement           | Description                       |
+===============+=======================+===================================+
| ``Sequence``  | |Forward Sequence|    | A sequence to test.               |
+---------------+-----------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s``:


.. parsed-literal::

    typedef empty<s>::type c; 

:Return type:
    Boolean |Integral Constant|.

:Semantics:
    Equivalent to ``typedef is_same< begin<s>::type,end<s>::type >::type c;``.

:Postcondition:
    ``empty<s>::value == ( size<s>::value == 0 )``.



Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef range_c<int,0,0> empty_range;
    typedef vector<long,float,double> types;
    
    BOOST_MPL_ASSERT( empty<empty_range> );
    BOOST_MPL_ASSERT_NOT( empty<types> );


See also
--------

|Forward Sequence|, |Integral Constant|, |size|, |begin| / |end|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
