.. Sequences/Classes//range_c |60

range_c
=======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T
        , T Start
        , T Finish
        >
    struct range_c
    {
        typedef integral_c<T,Start> start;
        typedef integral_c<T,Finish> finish;
        // |unspecified|
        // |...|
    };


Description
-----------

``range_c`` is a sorted |Random Access Sequence| of |Integral Constant|\ s. Note 
that because it is not an |Extensible Sequence|, sequence-building 
intrinsic metafunctions such as ``push_front`` and transformation algorithms
such as ``replace`` are not directly applicable |--| to be able to use
them, you'd first need to copy the content of the range into a more suitable
sequence.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/range_c.hpp>


Model of
--------

|Random Access Sequence|


Expression semantics
--------------------

In the following table, ``r`` is an instance of ``range_c``, ``n`` is an |Integral Constant|,
``T`` is an arbitrary integral type, and ``n`` and ``m`` are integral constant values of type ``T``.

+-------------------------------+-----------------------------------------------------------+
| Expression                    | Semantics                                                 |
+===============================+===========================================================+
| .. parsed-literal::           | A sorted |Random Access Sequence| of integral constant    |
|                               | wrappers for the half-open range of values [\ ``n``,      |
|    ``range_c<T,n,m>``         | ``m``): ``integral_c<T,n>``, ``integral_c<T,n+1>``,...    |
|    ``range_c<T,n,m>::type``   | ``integral_c<T,m-1>``.                                    |
|                               |                                                           |
+-------------------------------+-----------------------------------------------------------+
| ``begin<r>::type``            | An iterator pointing to the beginning of ``r``;           |
|                               | see |Random Access Sequence|.                             |
+-------------------------------+-----------------------------------------------------------+
| ``end<r>::type``              | An iterator pointing to the end of ``r``;                 |
|                               | see |Random Access Sequence|.                             |
+-------------------------------+-----------------------------------------------------------+
| ``size<r>::type``             | The size of ``r``; see |Random Access Sequence|.          |
+-------------------------------+-----------------------------------------------------------+
| ``empty<r>::type``            | |true if and only if| ``r`` is empty; see                 |
|                               | |Random Access Sequence|.                                 |
+-------------------------------+-----------------------------------------------------------+
| ``front<r>::type``            | The first element in ``r``; see                           |
|                               | |Random Access Sequence|.                                 |
+-------------------------------+-----------------------------------------------------------+
| ``back<r>::type``             | The last element in ``r``; see                            |
|                               | |Random Access Sequence|.                                 |
+-------------------------------+-----------------------------------------------------------+
| ``at<r,n>::type``             | The ``n``\ th element from the beginning of ``r``; see    |
|                               | |Random Access Sequence|.                                 |
+-------------------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::
    
    typedef range_c<int,0,0> range0;
    typedef range_c<int,0,1> range1;
    typedef range_c<int,0,10> range10;
    
    BOOST_MPL_ASSERT_RELATION( size<range0>::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( size<range1>::value, ==, 1 );
    BOOST_MPL_ASSERT_RELATION( size<range10>::value, ==, 10 );
    
    BOOST_MPL_ASSERT(( empty<range0> ));
    BOOST_MPL_ASSERT_NOT(( empty<range1> ));
    BOOST_MPL_ASSERT_NOT(( empty<range10> ));
    
    BOOST_MPL_ASSERT(( is_same< begin<range0>::type, end<range0>::type > ));
    BOOST_MPL_ASSERT_NOT(( is_same< begin<range1>::type, end<range1>::type > ));
    BOOST_MPL_ASSERT_NOT(( is_same< begin<range10>::type, end<range10>::type > ));
    
    BOOST_MPL_ASSERT_RELATION( front<range1>::type::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( back<range1>::type::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( front<range10>::type::value, ==, 0 );
    BOOST_MPL_ASSERT_RELATION( back<range10>::type::value, ==, 9 );


See also
--------

|Sequences|, |Random Access Sequence|, |vector_c|, |set_c|, |list_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
