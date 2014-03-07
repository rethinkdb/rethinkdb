.. Macros/Asserts//BOOST_MPL_ASSERT_RELATION

BOOST_MPL_ASSERT_RELATION
=========================

Synopsis
--------

.. parsed-literal::
    
    #define BOOST_MPL_ASSERT_RELATION( x, relation, y ) \\
        |unspecified-token-seq| \\
    /\*\*/



Description
-----------

A specialized assertion macro for checking numerical conditions. Generates 
a compilation error when the condition ``( x relation y )`` 
doesn't hold.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/assert.hpp>


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``x``         | An integral constant              | Left operand of the checked relation.         |
+---------------+-----------------------------------+-----------------------------------------------+
| ``y``         | An integral constant              | Right operand of the checked relation.        |
+---------------+-----------------------------------+-----------------------------------------------+
| ``relation``  | A C++ operator token              | An operator token for the relation being      |
|               |                                   | checked.                                      |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any integral constants ``x``, ``y`` and a legal C++ operator token ``op``:


.. parsed-literal::

    BOOST_MPL_ASSERT_RELATION( x, op, y );

:Return type:
    None.

:Semantics:
    Generates a compilation error if ``( x op y ) != true``, otherwise
    has no effect. 
    
    When possible within the compiler's diagnostic capabilities,
    the error message will include a name of the relation being checked,
    the actual values of both operands, and have a general form of:

    .. parsed-literal::
    
        |...| \*\*\*\*\*\*\*\*\*\*\*\*\ |...|\ assert_relation<op, x, y>::\*\*\*\*\*\*\*\*\*\*\*\*) |...|


Example
-------

::
    
    template< typename T, typename U > struct my
    {
        // ...
        BOOST_MPL_ASSERT_RELATION( sizeof(T), <, sizeof(U) );
    };
    
    my<char[50],char[10]> test;

    // In instantiation of `my<char[50], char[10]>':
    //   instantiated from here
    // conversion from `
    //   mpl_::failed************mpl_::assert_relation<less, 50, 10>::************' 
    //   to non-scalar type `mpl_::assert<false>' requested


See also
--------

|Asserts|, |BOOST_MPL_ASSERT|, |BOOST_MPL_ASSERT_NOT|, |BOOST_MPL_ASSERT_MSG|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
