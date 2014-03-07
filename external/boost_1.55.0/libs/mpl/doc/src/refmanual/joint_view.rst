.. Sequences/Views//joint_view

joint_view
==========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence1
        , typename Sequence2
        >
    struct joint_view
    {
        // |unspecified|
        // |...|
    };



Description
-----------

A view into the sequence of elements formed by concatenating ``Sequence1`` 
and ``Sequence2`` elements. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/joint_view.hpp>


Model of
--------

* |Forward Sequence|


Parameters
----------

+-----------------------+---------------------------+-----------------------------------+
| Parameter             | Requirement               | Description                       |
+=======================+===========================+===================================+
| ``Sequence1``,        | |Forward Sequence|        | Sequences to create a view on.    |
| ``Sequence2``         |                           |                                   |
+-----------------------+---------------------------+-----------------------------------+

Expression semantics
--------------------

|Semantics disclaimer...| |Forward Sequence|.

In the following table, ``v`` is an instance of ``joint_view``, ``s1`` and ``s2`` are arbitrary 
|Forward Sequence|\ s.

+-------------------------------+-----------------------------------------------------------+
| Expression                    | Semantics                                                 |
+===============================+===========================================================+
| .. parsed-literal::           | A lazy |Forward Sequence| of all the elements in the      |
|                               | ranges |begin/end<s1>|, |begin/end<s2>|.                  |
|    joint_view<s1,s2>          |                                                           |
|    joint_view<s1,s2>::type    |                                                           |
+-------------------------------+-----------------------------------------------------------+
| ``size<v>::type``             | The size of ``v``;                                        |
|                               | ``size<v>::value == size<s1>::value + size<s2>::value``;  |
|                               | linear complexity; see |Forward Sequence|.                |
+-------------------------------+-----------------------------------------------------------+

Example
-------

.. parsed-literal::
    
    typedef joint_view<
          range_c<int,0,10>
        , range_c<int,10,15>
        > numbers;
    
    BOOST_MPL_ASSERT(( equal< numbers, range_c<int,0,15> > ));


See also
--------

|Sequences|, |Views|, |filter_view|, |transform_view|, |zip_view|, |iterator_range|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
