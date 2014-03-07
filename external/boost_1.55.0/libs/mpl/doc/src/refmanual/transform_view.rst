.. Sequences/Views//transform_view

transform_view
==============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename F
        >
    struct transform_view
    {
        // |unspecified|
        // |...|
    };


Description
-----------

A view the full range of ``Sequence``\ 's transformed elements.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/transform_view.hpp>


Model of
--------

* |Forward Sequence|


Parameters
----------

+---------------+-------------------------------+-------------------------------+
| Parameter     | Requirement                   | Description                   |
+===============+===============================+===============================+
| ``Sequence``  | |Forward Sequence|            | A sequence to wrap.           |
+---------------+-------------------------------+-------------------------------+
| ``F``         | Unary |Lambda Expression|     | A transformation.             |
+---------------+-------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Forward Sequence|.

In the following table, ``v`` is an instance of ``transform_view``, ``s`` is an arbitrary 
|Forward Sequence|, and ``f`` is an unary |Lambda Expression|.

+-----------------------------------+-----------------------------------------------------------+
| Expression                        | Semantics                                                 |
+===================================+===========================================================+
| .. parsed-literal::               | A lazy |Forward Sequence| such that for each ``i`` in the |
|                                   | range |begin/end<v>| and each ``j`` in for in the range   |
|    transform_view<s,f>            | |begin/end<s>| ``deref<i>::type`` is identical to         |
|    transform_view<s,f>::type      | ``apply< f, deref<j>::type >::type``.                     |
+-----------------------------------+-----------------------------------------------------------+
| ``size<v>::type``                 | The size of ``v``;                                        |
|                                   | ``size<v>::value == size<s>::value``;                     |
|                                   | linear complexity; see |Forward Sequence|.                |
+-----------------------------------+-----------------------------------------------------------+


Example
-------

Find the largest type in a sequence.

.. parsed-literal::
    
    typedef vector<int,long,char,char[50],double> types;
    typedef max_element<
          transform_view< types, size_of<_> >
        >::type iter;

    BOOST_MPL_ASSERT_RELATION( deref<iter>::type::value, ==, 50 );


See also
--------

|Sequences|, |Views|, |filter_view|, |joint_view|, |zip_view|, |iterator_range|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
