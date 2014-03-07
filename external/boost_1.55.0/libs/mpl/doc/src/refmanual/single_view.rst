.. Sequences/Views//single_view

single_view
===========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T
        >
    struct single_view
    {
        // |unspecified|
        // |...|
    };



Description
-----------

A view onto an arbitrary type ``T`` as on a single-element sequence. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/single_view.hpp>


Model of
--------

* |Random Access Sequence|


Parameters
----------

+---------------+-------------------+-----------------------------------------------+
| Parameter     | Requirement       | Description                                   |
+===============+===================+===============================================+
| ``T``         | Any type          | The type to be wrapped in a sequence.         |
+---------------+-------------------+-----------------------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Random Access Sequence|.

In the following table, ``v`` is an instance of ``single_view``, ``x`` is an arbitrary type.

+-------------------------------+-----------------------------------------------------------+
| Expression                    | Semantics                                                 |
+===============================+===========================================================+
| .. parsed-literal::           | A single-element |Random Access Sequence| ``v`` such that |
|                               | ``front<v>::type`` is identical to ``x``.                 |
|    single_view<x>             |                                                           |
|    single_view<x>::type       |                                                           |
+-------------------------------+-----------------------------------------------------------+
| ``size<v>::type``             | The size of ``v``; ``size<v>::value == 1``;               |
|                               | see |Random Access Sequence|.                             |
+-------------------------------+-----------------------------------------------------------+

Example
-------

.. parsed-literal::
    
    typedef single_view<int> view;
    typedef begin<view>::type first;
    typedef end<view>::type last;
    
    BOOST_MPL_ASSERT(( is_same< deref<first>::type,int > ));
    BOOST_MPL_ASSERT(( is_same< next<first>::type,last > ));
    BOOST_MPL_ASSERT(( is_same< prior<last>::type,first > ));
    
    BOOST_MPL_ASSERT_RELATION( size<view>::value, ==, 1 );


See also
--------

|Sequences|, |Views|, |iterator_range|, |filter_view|, |transform_view|, |joint_view|, |zip_view|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
