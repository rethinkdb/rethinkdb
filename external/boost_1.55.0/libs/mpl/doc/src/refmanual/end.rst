.. Sequences/Intrinsic Metafunctions//end

end
===

Synopsis
--------

.. parsed-literal::
    
    template<
          typename X
        >
    struct end
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the sequence's past-the-end iterator. If the argument is not a 
|Forward Sequence|, returns |void_|.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/begin_end.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-------------------+-----------------------------------------------+
| Parameter     | Requirement       | Description                                   |
+===============+===================+===============================================+
| ``X``         | Any type          | A type whose end iterator, if any, will be    |
|               |                   | returned.                                     |
+---------------+-------------------+-----------------------------------------------+


Expression semantics
--------------------

For any arbitrary type ``x``:

.. parsed-literal::

    typedef end<x>::type last; 

:Return type:
    |Forward Iterator| or |void_|.

:Semantics:
    If ``x`` is |Forward Sequence|, ``last`` is an iterator pointing one past the 
    last element in ``s``; otherwise ``last`` is |void_|.

:Postcondition:
    If ``last`` is an iterator, it is past-the-end. 


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector<long> v;
    typedef begin<v>::type first;
    typedef end<v>::type last;

    BOOST_MPL_ASSERT(( is_same< next<first>::type, last > ));


See also
--------

|Iterators|, |Forward Sequence|, |begin|, |end|, |next|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
