.. Algorithms/Transformation Algorithms//sort |95

sort
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq
        , typename Pred = less<_1,_2>
        , typename In = |unspecified|
        >
    struct sort
    {
        typedef |unspecified| type;
    };


Description
-----------

Returns a new sequence of all elements in the range |begin/end<Seq>| sorted according
to the ordering relation ``Pred``. 

|transformation algorithm disclaimer|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/sort.hpp>


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
| ``Pred``          | Binary |Lambda Expression|        | An ordering relation.         |
+-------------------+-----------------------------------+-------------------------------+
| ``In``            | |Inserter|                        | An inserter.                  |
+-------------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, a binary |Lambda Expression| ``pred``, and an 
|Inserter| ``in``:


.. parsed-literal::

    typedef sort<s,pred,in>::type r;

:Return type:
    A type.
    
:Semantics:
    If ``size<s>::value <= 1``, equivalent to

    .. parsed-literal::

        typedef copy<s,in>::type r;
    
    
    otherwise equivalent to 
    
    .. parsed-literal::

        typedef back_inserter< vector<> > aux_in;
        typedef lambda<pred>::type p;

        typedef begin<s>::type pivot;
        typedef partition<
              iterator_range< next<pivot>::type, end<s>::type >
            , apply_wrap2<p,_1,deref<pivot>::type>
            , aux_in
            , aux_in
            >::type partitioned;

        typedef sort<partitioned::first,p,aux_in >::type part1;
        typedef sort<partitioned::second,p,aux_in >::type part2;
        
        typedef copy<
              joint_view< 
                  joint_view<part1,single_view< deref<pivot>::type > >
                , part2
                >
            , in
            >::type r;


Complexity
----------

Average *O(n log(n))* where *n* == ``size<s>::value``, quadratic at worst.

Example
-------

.. parsed-literal::
    
    typedef vector_c<int,3,4,0,-5,8,-1,7> numbers;
    typedef vector_c<int,-5,-1,0,3,4,7,8> expected;
    typedef sort<numbers>::type result;

    BOOST_MPL_ASSERT(( equal< result, expected, equal_to<_,_> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |partition|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
