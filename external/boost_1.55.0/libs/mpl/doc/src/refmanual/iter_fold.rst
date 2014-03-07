.. Algorithms/Iteration Algorithms//iter_fold

iter_fold
=========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename State
        , typename ForwardOp
        >
    struct iter_fold
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of the successive application of binary ``ForwardOp`` to the result 
of the previous ``ForwardOp`` invocation (``State`` if it's the first call) and each 
iterator in the range [``begin<Sequence>::type``, ``end<Sequence>::type``) in order.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/iter_fold.hpp>



Parameters
----------

+---------------+-------------------------------+---------------------------------------------------+
| Parameter     | Requirement                   | Description                                       |
+===============+===============================+===================================================+
| ``Sequence``  | |Forward Sequence|            | A sequence to iterate.                            |
+---------------+-------------------------------+---------------------------------------------------+
| ``State``     | Any type                      | The initial state for the first ``ForwardOp``     |
|               |                               | application.                                      |
+---------------+-------------------------------+---------------------------------------------------+
| ``ForwardOp`` | Binary |Lambda Expression|    | The operation to be executed on forward           |
|               |                               | traversal.                                        |
+---------------+-------------------------------+---------------------------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s``, binary |Lambda Expression| ``op``, and an 
arbitrary type ``state``:


.. parsed-literal::

    typedef iter_fold<s,state,op>::type t; 

:Return type:
    A type.

:Semantics:
    Equivalent to 
    
    .. parsed-literal::

        typedef begin<s>::type i\ :sub:`1`;
        typedef apply<op,state,i\ :sub:`1`>::type state\ :sub:`1`;
        typedef next<i\ :sub:`1`>::type i\ :sub:`2`;
        typedef apply<op,state\ :sub:`1`,i\ :sub:`2`>::type state\ :sub:`2`;
        |...|
        typedef apply<op,state\ :sub:`n-1`,i\ :sub:`n`>::type state\ :sub:`n`; 
        typedef next<i\ :sub:`n`>::type last; 
        typedef state\ :sub:`n` t;
        
    where ``n == size<s>::value`` and ``last`` is identical to ``end<s>::type``; equivalent 
    to ``typedef state t;`` if ``empty<s>::value == true``. 



Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``op``. 


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,5,-1,0,7,2,0,-5,4> numbers;
    typedef iter_fold<
          numbers
        , begin<numbers>::type
        , if_< less< deref<_1>, deref<_2> >,_2,_1 >
        >::type max_element_iter;
    
    BOOST_MPL_ASSERT_RELATION( deref<max_element_iter>::type::value, ==, 7 );



See also
--------

|Algorithms|, |reverse_iter_fold|, |fold|, |reverse_fold|, |copy|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
