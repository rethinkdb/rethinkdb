.. Sequences/Views//filter_view

filter_view
===========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Pred
        >
    struct filter_view
    {
        // |unspecified|
        // |...|
    };



Description
-----------

A view into a subset of ``Sequence``\ 's elements satisfying the predicate ``Pred``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/filter_view.hpp>


Model of
--------

* |Forward Sequence|


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``Sequence``  | |Forward Sequence|                | A sequence to wrap.                           |
+---------------+-----------------------------------+-----------------------------------------------+
| ``Pred``      | Unary |Lambda Expression|         | A filtering predicate.                        |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

Semantics of an expression is defined only where it differs from, or is not 
defined in |Forward Sequence|.

In the following table, ``v`` is an instance of ``filter_view``, ``s`` is an arbitrary 
|Forward Sequence|, ``pred`` is an unary |Lambda Expression|.

+---------------------------------------+-----------------------------------------------------------+
| Expression                            | Semantics                                                 |
+=======================================+===========================================================+
| .. parsed-literal::                   | A lazy |Forward Sequence| sequence of all the elements in |
|                                       | the range |begin/end<s>| that satisfy the predicate       |
|    filter_view<s,pred>                | ``pred``.                                                 |
|    filter_view<s,pred>::type          |                                                           |
+---------------------------------------+-----------------------------------------------------------+
| ``size<v>::type``                     | The size of ``v``;                                        |
|                                       | ``size<v>::value == count_if<s,pred>::value``;            |
|                                       | linear complexity; see |Forward Sequence|.                |
+---------------------------------------+-----------------------------------------------------------+


Example
-------

Find the largest floating type in a sequence.

.. parsed-literal::
    
    typedef vector<int,float,long,float,char[50],long double,char> types;
    typedef max_element<
          transform_view< filter_view< types,boost::is_float<_> >, size_of<_> >
        >::type iter;
    
    BOOST_MPL_ASSERT(( is_same< deref<iter::base>::type, long double > ));


See also
--------

|Sequences|, |Views|, |transform_view|, |joint_view|, |zip_view|, |iterator_range|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
