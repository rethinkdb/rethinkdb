.. .. Algorithms/Iteration Algorithms

.. UNFINISHED: Expression semantics and everything that follows


iter_fold_if
============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename State
        , typename ForwardOp
        , typename ForwardPred
        , typename BackwardOp = |unspecified|
        , typename BackwardPred = |unspecified|
        >
    struct iter_fold_if
    {
        typedef |unspecified| type;
    };


Description
-----------

Returns the result of the successive application of binary ``ForwardOp`` to the result 
of the previous ``ForwardOp`` invocation (``State`` if it's the first call) and each 
iterator in the sequence range determined by ``ForwardPred`` predicate. If ``BackwardOp``
is provided, it's similarly applied on backward traversal to the result of the 
previous ``BackwardOp`` invocation (the last result returned by ``ForwardOp` if it's 
the first call) and each iterator in the sequence range determined by 
``BackwardPred`` predicate.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/iter_fold_if.hpp>


Parameters
----------

+-------------------+-------------------------------+-----------------------------------------------+
| Parameter         | Requirement                   | Description                                   |
+===============+===================================+===============================================+
| ``Sequence``      | |Forward Sequence|            | A sequence to iterate.                        |
+-------------------+-------------------------------+-----------------------------------------------+
| ``State``         | Any type                      | The initial state for the first ``BackwardOp``|
|                   |                               | / ``ForwardOp`` application.                  |
+-------------------+-------------------------------+-----------------------------------------------+
| ``ForwardOp``     | Binary |Lambda Expression|    | The operation to be executed on forward       |
|                   |                               | traversal.                                    |
+-------------------+-------------------------------+-----------------------------------------------+
| ``ForwardPred``   | Binary |Lambda Expression|    | The forward traversal predicate.              |
+-------------------+-------------------------------+-----------------------------------------------+
| ``BackwardOp``    | Binary |Lambda Expression|    | The operation to be executed on backward      |
|                   |                               | traversal.                                    |
+-------------------+-------------------------------+-----------------------------------------------+
| ``BackwardPred``  | Binary |Lambda Expression|    | The backward traversal predicate.             |
+-------------------+-------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s``, binary |Lambda Expression| ``op``, and an 
arbitrary type ``state``:


.. parsed-literal::

    typedef iter_fold<Sequence,T,Op>::type t; 

:Return type:
    A type 

:Semantics:
    Equivalent to 
    
    .. parsed-literal::

        typedef lambda<Op>::type op;
        typedef begin<Sequence>::type i1;
        typedef apply<op,T,i1>::type t1;
        typedef i1::next i2;
        typedef apply<op,t1,i2>::type t2;
        ...
        typedef apply<op,T,in>::type tn; 
        typedef in::next last; 
        typedef tn t
        
    where ``n == size<Sequence>::value`` and ``last`` is identical to ``end<Sequence>::type``; 
        
    Equivalent to ``typedef T t;`` if the sequence is empty. 



Complexity
----------

Linear. Exactly ``size<Sequence>::value`` applications of ``ForwardOp``. 


Example
-------

.. parsed-literal::
    
    typedef list_c<int,5,-1,0,7,2,0,-5,4> numbers;
    typedef iter_fold<
          numbers
        , begin<numbers>::type
        , if_< less< deref<_1>, deref<_2> >,_2,_1 >
        >::type max_element_iter;
    
    BOOST_STATIC_ASSERT(max_element_iter::type::value == 7);



See also
--------

Algorithms, ``iter_fold_backward``, ``fold``, ``fold_backward``, ``copy``, ``copy_backward``


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
