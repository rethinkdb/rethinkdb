.. Algorithms/Iteration Algorithms//fold

fold
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename State
        , typename ForwardOp
        >
    struct fold
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of the successive application of binary ``ForwardOp`` to the 
result of the previous ``ForwardOp`` invocation (``State`` if it's the first call) 
and every element of the sequence in the range |begin/end<Sequence>| in order.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/fold.hpp>


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

For any |Forward Sequence| ``s``, binary |Lambda Expression| ``op``, and arbitrary type ``state``:


.. parsed-literal::

    typedef fold<s,state,op>::type t; 

:Return type:
    A type.

:Semantics:
    Equivalent to
        
    .. parsed-literal::
    
        typedef iter_fold<
              s
            , state
            , apply_wrap2< lambda<op>::type, _1, deref<_2> >
            >::type t; 


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``op``. 


Example
-------

.. parsed-literal::
    
    typedef vector<long,float,short,double,float,long,long double> types;
    typedef fold<
          types
        , int_<0>
        , if_< is_float<_2>,next<_1>,_1 >
        >::type number_of_floats;
    
    BOOST_MPL_ASSERT_RELATION( number_of_floats::value, ==, 4 );


See also
--------

|Algorithms|, |accumulate|, |reverse_fold|, |iter_fold|, |reverse_iter_fold|, |copy|, |copy_if|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
