.. Iterators/Iterator Metafunctions//prior |40

prior
=====

Synopsis
--------

.. parsed-literal::

    template<
          typename Iterator
        >
    struct prior
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the previous iterator in the sequence. |Note:| ``prior`` has a number of 
overloaded meanings, depending on the type of its argument. For instance,
if ``X`` is an |Integral Constant|, ``prior<X>`` returns an decremented 
|Integral Constant| of the same type. The following specification is 
iterator-specific. Please refer to the corresponding concept's
documentation for the details of the alternative semantics |-- end note|. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/next_prior.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------+
| Parameter     | Requirement               | Description                       |
+===============+===========================+===================================+
| ``Iterator``  | |Bidirectional Iterator|. | An iterator to decrement.         |
+---------------+---------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Bidirectional Iterator|\ s ``iter``:


.. parsed-literal::

    typedef prior<iter>::type j; 

:Return type:
    |Bidirectional Iterator|.

:Precondition:
    ``iter`` is decrementable.

:Semantics:
    ``j`` is an iterator pointing to the previous element in the sequence. 
    If ``iter`` is a user-defined iterator, the library-provided default 
    implementation is equivalent to

    .. parsed-literal::
    
        typedef iter::prior j;


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,1> v;
    typedef begin<v>::type first;
    typedef end<v>::type last;
    
    BOOST_MPL_ASSERT(( is_same< prior<last>::type, first > ));


See also
--------

|Iterators|, |begin| / |end|, |next|, |deref|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
