.. Algorithms/Iteration Algorithms//reverse_fold

reverse_fold
============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename State
        , typename BackwardOp
        , typename ForwardOp = _1
        >
    struct reverse_fold
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of the successive application of binary ``BackwardOp`` to the 
result of the previous ``BackwardOp`` invocation (``State`` if it's the first call) 
and every element in the range [``begin<Sequence>::type``, ``end<Sequence>::type``) in 
reverse order. If ``ForwardOp`` is provided, then it is applied on forward 
traversal to form the result that is passed to the first ``BackwardOp`` call.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/reverse_fold.hpp>



Parameters
----------

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

    typedef reverse_fold< s,state,backward_op >::type t; 

:Return type:
    A type 

:Semantics:
    Equivalent to

    .. parsed-literal::
    
        typedef lambda<backward_op>::type op; 
        typedef reverse_iter_fold< 
              s
            , state
            , apply_wrap2< op, _1, deref<_2> >
            >::type t; 


.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. parsed-literal::

    typedef reverse_fold< s,state,backward_op,forward_op >::type t; 


:Return type:
    A type.

:Semantics:
    Equivalent to

    .. parsed-literal::
    
        typedef reverse_fold<
              Sequence
            , fold<s,state,forward_op>::type
            , backward_op
            >::type t;        


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``backward_op`` and ``forward_op``. 


Example
-------

Remove non-negative elements from a sequence [#reverse_fold_note]_.

.. parsed-literal::
    
    typedef list_c<int,5,-1,0,-7,-2,0,-5,4> numbers;
    typedef list_c<int,-1,-7,-2,-5> negatives;
    typedef reverse_fold<
          numbers
        , list_c<int>
        , if_< less< _2,int_<0> >, push_front<_1,_2,>, _1 >
        >::type result;
    
    BOOST_MPL_ASSERT(( equal< negatives,result > ));


.. [#reverse_fold_note] See ``remove_if`` for a more compact way to do this.


See also
--------

|Algorithms|, |fold|, |reverse_iter_fold|, |iter_fold|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
