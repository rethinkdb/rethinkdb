.. Algorithms/Transformation Algorithms//reverse_partition |185

reverse_partition
=================

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq
        , typename Pred
        , typename In1 = |unspecified|
        , typename In2 = |unspecified|
        >
    struct reverse_partition
    {
        typedef |unspecified| type;
    };


Description
-----------

Returns a pair of sequences together containing all elements in the range 
|begin/end<Seq>| split into two groups based on the predicate ``Pred``.
``reverse_partition`` is a synonym for |reverse_stable_partition|.

|transformation algorithm disclaimer|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/partition.hpp>


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

    typedef reverse_partition<s,pred,in1,in2>::type r;

:Return type:
    A |pair|.
    
:Semantics:
    Equivalent to 

    .. parsed-literal::

        typedef reverse_stable_partition<s,pred,in1,in2>::type r;


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``pred``, and ``size<s>::value`` 
of summarized ``in1::operation`` / ``in2::operation`` applications. 


Example
-------

.. parsed-literal::
    
    template< typename N > struct is_odd : bool_<(N::value % 2)> {};

    typedef partition<
          range_c<int,0,10> 
        , is_odd<_1>
        , back_inserter< vector<> >
        , back_inserter< vector<> >
        >::type r;

    BOOST_MPL_ASSERT(( equal< r::first, vector_c<int,9,7,5,3,1> > ));
    BOOST_MPL_ASSERT(( equal< r::second, vector_c<int,8,6,4,2,0> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |partition|, |reverse_stable_partition|, |sort|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
