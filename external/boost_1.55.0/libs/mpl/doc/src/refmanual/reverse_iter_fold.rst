.. Algorithms/Iteration Algorithms//reverse_iter_fold

reverse_iter_fold
=================

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename State
        , typename BackwardOp
        , typename ForwardOp = _1
        >
    struct reverse_iter_fold
    {
        typedef |unspecified|  type;
    };



Description
-----------

Returns the result of the successive application of binary ``BackwardOp`` to the 
result of the previous ``BackwardOp`` invocation (``State`` if it's the first call) 
and each iterator in the range [``begin<Sequence>::type``, ``end<Sequence>::type``) 
in reverse order. If ``ForwardOp`` is provided, then it's applied on forward 
traversal to form the result which is passed to the first ``BackwardOp`` call.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/reverse_iter_fold.hpp>



Parameters
----------

+---------------+-------------------------------+-----------------------------------------------+
| Parameter     | Requirement                   | Description                                   |
+===============+===============================+===============================================+
| ``Sequence``  | |Forward Sequence|            | A sequence to iterate.                        |
+---------------+-------------------------------+-----------------------------------------------+
| ``State``     | Any type                      | The initial state for the first ``BackwardOp``|
|               |                               | / ``ForwardOp`` application.                  |
+---------------+-------------------------------+-----------------------------------------------+
| ``BackwardOp``| Binary |Lambda Expression|    | The operation to be executed on backward      |
|               |                               | traversal.                                    |
+---------------+-------------------------------+-----------------------------------------------+
| ``ForwardOp`` | Binary |Lambda Expression|    | The operation to be executed on forward       |
|               |                               | traversal.                                    |
+---------------+-------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s``, binary |Lambda Expression| ``backward_op`` and ``forward_op``, 
and arbitrary type ``state``:


.. parsed-literal::

    typedef reverse_iter_fold< s,state,backward_op >::type t; 

:Return type:
    A type.

:Semantics:
    Equivalent to 

    .. parsed-literal::

        typedef begin<s>::type i\ :sub:`1`;
        typedef next<i\ :sub:`1`>::type i\ :sub:`2`;
        |...|
        typedef next<i\ :sub:`n`>::type last;
        typedef apply<backward_op,state,i\ :sub:`n`>::type state\ :sub:`n`;
        typedef apply<backward_op,state\ :sub:`n`,i\ :sub:`n-1`>::type state\ :sub:`n-1`; 
        |...|
        typedef apply<backward_op,state\ :sub:`2`,i\ :sub:`1`>::type state\ :sub:`1`; 
        typedef state\ :sub:`1` t;
        
    where ``n == size<s>::value`` and ``last`` is identical to ``end<s>::type``; equivalent 
    to ``typedef state t;`` if ``empty<s>::value == true``. 


.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. parsed-literal::

    typedef reverse_iter_fold< s,state,backward_op,forward_op >::type t; 

:Return type:
    A type.

:Semantics:
    Equivalent to 
    
    .. parsed-literal::
    
        typedef reverse_iter_fold<
              Sequence
            , iter_fold<s,state,forward_op>::type
            , backward_op
            >::type t; 


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``backward_op`` and ``forward_op``. 


Example
-------

Build a list of iterators to the negative elements in a sequence.

.. parsed-literal::
    
    typedef vector_c<int,5,-1,0,-7,-2,0,-5,4> numbers;
    typedef list_c<int,-1,-7,-2,-5> negatives;
    typedef reverse_iter_fold<
          numbers
        , list<>
        , if_< less< deref<_2>,int_<0> >, push_front<_1,_2>, _1 >
        >::type iters;
    
    BOOST_MPL_ASSERT(( equal< 
          negatives
        , transform_view< iters,deref<_1> >
        > ));


See also
--------

|Algorithms|, |iter_fold|, |reverse_fold|, |fold|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
