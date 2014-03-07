.. Algorithms/Transformation Algorithms//reverse_transform |130

reverse_transform
=================

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq
        , typename Op
        , typename In = |unspecified|
        >
    struct reverse_transform
    {
        typedef |unspecified| type;
    };

    template<
          typename Seq1
        , typename Seq2
        , typename BinaryOp
        , typename In = |unspecified|
        >
    struct reverse_transform
    {
        typedef |unspecified| type;
    };


Description
-----------

``reverse_transform`` is an |overloaded name|:

* ``reverse_transform<Seq,Op>`` returns a reversed, transformed copy of the 
  original sequence produced by applying an unary transformation ``Op`` to 
  every element in the |begin/end<Sequence>| range.

* ``reverse_transform<Seq1,Seq2,Op>`` returns a new sequence produced by applying a
  binary transformation ``BinaryOp`` to a pair of elements (e\ :sub:`1`, e2\ :sub:`1`) 
  from the corresponding |begin/end<Seq1>| and |begin/end<Seq2>| ranges in reverse
  order.

|transformation algorithm disclaimer|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/transform.hpp>


Model of
--------

|Reversible Algorithm|


Parameters
----------

+-------------------+-----------------------------------+-----------------------------------+
| Parameter         | Requirement                       | Description                       |
+===================+===================================+===================================+
| ``Sequence``,     | |Forward Sequence|                | Sequences to transform.           |
| ``Seq1``, ``Seq2``|                                   |                                   |
+-------------------+-----------------------------------+-----------------------------------+
| ``Op``,           | |Lambda Expression|               | A transformation.                 |
| ``BinaryOp``      |                                   |                                   |
+-------------------+-----------------------------------+-----------------------------------+
| ``In``            | |Inserter|                        | An inserter.                      |
+-------------------+-----------------------------------+-----------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence|\ s ``s``, ``s1`` and ``s2``, |Lambda Expression|\ s ``op`` and ``op2``, 
and an |Inserter| ``in``:

.. parsed-literal::

    typedef reverse_transform<s,op,in>::type r; 

:Return type:
    A type.
    
:Postcondition:
    Equivalent to 
        
    .. parsed-literal::

        typedef lambda<op>::type f;
        typedef lambda<in::operation>::type in_op;
        
        typedef reverse_fold< 
              s
            , in::state
            , bind< in_op, _1, bind<f, _2> > 
            >::type r;


.. parsed-literal::

    typedef transform<s1,s2,op,in>::type r; 

:Return type:
    A type.
    
:Postcondition:
    Equivalent to 
        
    .. parsed-literal::

        typedef lambda<op2>::type f;
        typedef lambda<in::operation>::type in_op;

        typedef reverse_fold< 
              pair_view<s1,s2>
            , in::state
            , bind< 
                  in_op
                , _1
                , bind<f, bind<first<>,_2>, bind<second<>,_2> >
                > 
            >::type r;


Complexity
----------

Linear. Exactly ``size<s>::value`` / ``size<s1>::value`` applications of 
``op`` / ``op2`` and ``in::operation``.


Example
-------

.. parsed-literal::
    
    typedef vector<char,short,int,long,float,double> types;
    typedef vector<double*,float*,long*,int*,short*,char*> pointers;
    typedef reverse_transform< types,boost::add_pointer<_1> >::type result;
    
    BOOST_MPL_ASSERT(( equal<result,pointers> ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |transform|, |reverse_copy|, |replace_if|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
