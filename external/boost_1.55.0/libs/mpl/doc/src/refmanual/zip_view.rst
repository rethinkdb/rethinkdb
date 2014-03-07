.. Sequences/Views//zip_view

zip_view
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequences
        >
    struct zip_view
    {
        // |unspecified|
        // |...|
    };



Description
-----------

Provides a "zipped" view onto several sequences; that is, represents several 
sequences as a single sequence of elements each of which, in turn, 
is a sequence of the corresponding ``Sequences``\ ' elements. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/zip_view.hpp>



Model of
--------

* |Forward Sequence|


Parameters
----------

+---------------+-----------------------------------+-------------------------------+
| Parameter     | Requirement                       | Description                   |
+===============+===================================+===============================+
| ``Sequences`` | A |Forward Sequence| of           | Sequences to be "zipped".     |
|               | |Forward Sequence|\ s             |                               |
+---------------+-----------------------------------+-------------------------------+

Expression semantics
--------------------

|Semantics disclaimer...| |Forward Sequence|.

In the following table, ``v`` is an instance of ``zip_view``, ``seq`` a |Forward Sequence| of ``n``
|Forward Sequence|\ s.

+-------------------------------+-----------------------------------------------------------+
| Expression                    | Semantics                                                 |
+===============================+===========================================================+
| .. parsed-literal::           | A lazy |Forward Sequence| ``v`` such that for each ``i``  |
|                               | in |begin/end<v>| and for each ``j`` in                   |
|    zip_view<seq>              | [``begin<seq>::type``, ``end<seq>::type``)                |
|    zip_view<seq>::type        | ``deref<i>::type`` is identical to                        |
|                               | ``transform< deref<j>::type, deref<_1> >::type``.         |
+-------------------------------+-----------------------------------------------------------+
| ``size<v>::type``             | The size of ``v``; ``size<v>::value`` is equal to         |
|                               | ::                                                        |
|                               |                                                           |
|                               |   deref< min_element<                                     |
|                               |         transform_view< seq, size<_1> >                   |
|                               |       >::type >::type::value;                             |
|                               |                                                           |
|                               | linear complexity; see |Forward Sequence|.                |
+-------------------------------+-----------------------------------------------------------+


Example
-------

Element-wise sum of three vectors.

.. parsed-literal::
    
    typedef vector_c<int,1,2,3,4,5> v1;
    typedef vector_c<int,5,4,3,2,1> v2;
    typedef vector_c<int,1,1,1,1,1> v3;
    
    typedef transform_view<
          zip_view< vector<v1,v2,v3> >
        , unpack_args< plus<_1,_2,_3> >
        > sum;

    BOOST_MPL_ASSERT(( equal< sum, vector_c<int,7,7,7,7,7> > ));


See also
--------

|Sequences|, |Views|, |filter_view|, |transform_view|, |joint_view|, |single_view|, |iterator_range|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
