.. Algorithms/Transformation Algorithms//stable_partition |90

stable_partition
================

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq
        , typename Pred
        , typename In1 = |unspecified|
        , typename In2 = |unspecified|
        >
    struct stable_partition
    {
        typedef |unspecified| type;
    };


Description
-----------

Returns a pair of sequences together containing all elements in the range 
|begin/end<Seq>| split into two groups based on the predicate ``Pred``.
``stable_partition`` is guaranteed to preserve the relative order of the 
elements in the resulting sequences.


|transformation algorithm disclaimer|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/stable_partition.hpp>


Model of
--------

|Reversible Algorithm|


Parameters
----------

+-------------------+-----------------------------------+-------------------------------+
| Parameter         | Requirement                       | Description                   |
+===================+===================================+===============================+
| ``Seq``           | |Forward Sequence|                | An original sequence.         |
+-------------------+-----------------------------------+-------------------------------+
| ``Pred``          | Unary |Lambda Expression|         | A partitioning predicate.     |
+-------------------+-----------------------------------+-------------------------------+
| ``In1``, ``In2``  | |Inserter|                        | Output inserters.             |
+-------------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, an unary |Lambda Expression| ``pred``, and |Inserter|\ s 
``in1`` and ``in2``:


.. parsed-literal::

    typedef stable_partition<s,pred,in1,in2>::type r;

:Return type:
    A |pair|.
    
:Semantics:
    Equivalent to 
    
    .. parsed-literal::

        typedef lambda<pred>::type p;
        typedef lambda<in1::operation>::type in1_op;
        typedef lambda<in2::operation>::type in2_op;
        
        typedef fold<
              s
            , pair< in1::state, in2::state >
            , if_< 
                  apply_wrap\ ``1``\<p,_2>
                , pair< apply_wrap\ ``2``\<in1_op,first<_1>,_2>, second<_1> >
                , pair< first<_1>, apply_wrap\ ``2``\<in2_op,second<_1>,_2> >
                >
            >::type r;


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``pred``, and ``size<s>::value`` 
of summarized ``in1::operation`` / ``in2::operation`` applications. 


Example
-------

.. parsed-literal::
    
    template< typename N > struct is_odd : bool_<(N::value % 2)> {};

    typedef stable_partition<
          range_c<int,0,10> 
        , is_odd<_1>
        , back_inserter< vector<> >
        , back_inserter< vector<> >
        >::type r;

    BOOST_MPL_ASSERT(( equal< r::first, vector_c<int,1,3,5,7,9> > ));
    BOOST_MPL_ASSERT(( equal< r::second, vector_c<int,0,2,4,6,8> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |reverse_stable_partition|, |partition|, |sort|, |transform|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
