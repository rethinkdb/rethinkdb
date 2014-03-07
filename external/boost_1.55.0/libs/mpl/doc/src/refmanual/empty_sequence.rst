.. Sequences/Views//empty_sequence

empty_sequence
==============

Synopsis
--------

.. parsed-literal::
    
    struct empty_sequence
    {
        // |unspecified|
        // |...|
    };


Description
-----------

Represents a sequence containing no elements.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/empty_sequence.hpp>


Expression semantics
--------------------

|Semantics disclaimer...| |Random Access Sequence|.

In the following table, ``s`` is an instance of ``empty_sequence``.

+-------------------------------+-----------------------------------------------------------+
| Expression                    | Semantics                                                 |
+===============================+===========================================================+
| ``empty_sequence``            | An empty |Random Access Sequence|.                        |
+-------------------------------+-----------------------------------------------------------+
| ``size<s>::type``             | ``size<s>::value == 0``; see |Random Access Sequence|.    |
+-------------------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::

    typedef begin<empty_sequence>::type first;
    typedef end<empty_sequence>::type last;

    BOOST_MPL_ASSERT(( is_same<first,last> ));
    BOOST_MPL_ASSERT_RELATION( size<empty_sequence>::value, ==, 0 );

    typedef transform_view<
          empty_sequence
        , add_pointer<_>
        > empty_view;

    BOOST_MPL_ASSERT_RELATION( size<empty_sequence>::value, ==, 0 );


See also
--------

|Sequences|, |Views|, |vector|, |list|, |single_view|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
