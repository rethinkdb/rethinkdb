.. Sequences/Views//iterator_range

iterator_range
==============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename First
        , typename Last
        >
    struct iterator_range
    {
        // |unspecified|
        // |...|
    };


Description
-----------

A view into subset of sequence elements identified by a pair of iterators.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/fold.hpp>


Model of
--------

* `Forward`__, `Bidirectional`__, or |Random Access Sequence|, depending on the category
  of the underlaying iterators.

__ `Forward Sequence`_
__ `Bidirectional Sequence`_



Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``First``,    | |Forward Iterator|                | Iterators identifying the view's boundaries.  |
| ``Last``      |                                   |                                               |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Forward Sequence|.

In the following table, ``v`` is an instance of ``iterator_range``, ``first`` and ``last`` are 
iterators into a |Forward Sequence|, and [``first``, ``last``) form a valid range.

+-------------------------------------------+-------------------------------------------------------+
| Expression                                | Semantics                                             |
+===========================================+=======================================================+
| .. parsed-literal::                       | A lazy sequence all the elements in the range         |
|                                           | [``first``, ``last``).                                |
|    iterator_range<first,last>             |                                                       |
|    iterator_range<first,last>::type       |                                                       |
+-------------------------------------------+-------------------------------------------------------+

Example
-------

.. parsed-literal::
    
    typedef range_c<int,0,100> r;
    typedef advance_c< begin<r>::type,10 >::type first;
    typedef advance_c< end<r>::type,-10 >::type last;
    
    BOOST_MPL_ASSERT(( equal< 
          iterator_range<first,last>
        , range_c<int,10,90>
        > ));


See also
--------

|Sequences|, |Views|, |filter_view|, |transform_view|, |joint_view|, |zip_view|, |max_element|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
