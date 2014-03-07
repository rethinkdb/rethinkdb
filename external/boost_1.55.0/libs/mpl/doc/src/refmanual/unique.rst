.. Algorithms/Transformation Algorithms//unique |80

unique
======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq
        , typename Pred
        , typename In = |unspecified|
        >
    struct unique
    {
        typedef |unspecified| type;
    };


Description
-----------

Returns a sequence of the initial elements of every subrange of the
original sequence ``Seq`` whose elements are all the same. 

|transformation algorithm disclaimer|

Header
------

.. parsed-literal::
    
    #include <boost/mpl/unique.hpp>


Model of
--------

|Reversible Algorithm|


Parameters
----------

+---------------+-----------------------------------+-------------------------------+
| Parameter     | Requirement                       | Description                   |
+===============+===================================+===============================+
| ``Sequence``  | |Forward Sequence|                | An original sequence.         |
+---------------+-----------------------------------+-------------------------------+
| ``Pred``      | Binary |Lambda Expression|        | An equivalence relation.      |
+---------------+-----------------------------------+-------------------------------+
| ``In``        | |Inserter|                        | An inserter.                  |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, a binary |Lambda Expression| ``pred``, 
and an |Inserter| ``in``:


.. parsed-literal::

    typedef unique<s,pred,in>::type r; 

:Return type:
    A type.

:Semantics:
    If ``size<s>::value <= 1``, then equivalent to

    .. parsed-literal::
    
        typedef copy<s,in>::type r;
    
    otherwise equivalent to

    .. parsed-literal::

        typedef lambda<pred>::type p;
        typedef lambda<in::operation>::type in_op;
        typedef apply_wrap\ ``2``\<
              in_op
            , in::state
            , front<types>::type 
            >::type in_state;

        typedef fold<
              s
            , pair< in_state, front<s>::type >
            , eval_if< 
                  apply_wrap\ ``2``\<p, second<_1>, _2>
                , identity< first<_1> >
                , apply_wrap\ ``2``\<in_op, first<_1>, _2>
                >
            >::type::first r;


Complexity
----------

Linear. Performs exactly ``size<s>::value - 1`` applications of ``pred``, and at 
most ``size<s>::value`` insertions.


Example
-------

.. parsed-literal::
    
    typedef vector<int,float,float,char,int,int,int,double> types;
    typedef vector<int,float,char,int,double> expected;
    typedef unique< types, is_same<_1,_2> >::type result;
    
    BOOST_MPL_ASSERT(( equal< result,expected > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |reverse_unique|, |remove|, |copy_if|, |replace_if|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
