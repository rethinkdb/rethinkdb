.. Macros/Asserts//BOOST_MPL_ASSERT_MSG

BOOST_MPL_ASSERT_MSG
====================

Synopsis
--------

.. parsed-literal::
    
    #define BOOST_MPL_ASSERT_MSG( condition, message, types ) \\
        |unspecified-token-seq| \\
    /\*\*/


Description
-----------

Generates a compilation error with an embedded custom message when the condition 
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
| ``condition`` | An integral constant expression   | A condition to be asserted.                   |
+---------------+-----------------------------------+-----------------------------------------------+
| ``message``   | A legal identifier token          | A custom message in a form of a legal C++     |
|               |                                   | identifier token.                             |
+---------------+-----------------------------------+-----------------------------------------------+
| ``types``     | A legal function parameter list   | A parenthized list of types to be displayed   |
|               |                                   | in the error message.                         |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any integral constant expression ``expr``, legal C++ identifier ``message``, and 
arbitrary types ``t1``, ``t2``,... ``tn``:


.. parsed-literal::

    BOOST_MPL_ASSERT_MSG( expr, message, (t1, t2,... tn) );

:Return type:
    None.

:Precondition:
    ``t1``, ``t2``,... ``tn`` are non-``void``. 

:Semantics:
    Generates a compilation error if ``expr != true``, otherwise
    has no effect. 
    
    When possible within the compiler's diagnostic capabilities,
    the error message will include the ``message`` identifier and the parenthized 
    list of ``t1``, ``t2``,... ``tn`` types, and have a general form of:

    .. parsed-literal::
    
        |...| \*\*\*\*\*\*\*\*\*\*\*\*( |...|::message )\*\*\*\*\*\*\*\*\*\*\*\*)(t1, t2,... tn) |...|


.. parsed-literal::

    BOOST_MPL_ASSERT_MSG( expr, message, (types<t1, t2,... tn>) );

:Return type:
    None.

:Precondition:
    None.

:Semantics:
    Generates a compilation error if ``expr != true``, otherwise
    has no effect. 

    When possible within the compiler's diagnostics capabilities,
    the error message will include the ``message`` identifier and the list of 
    ``t1``, ``t2``,... ``tn`` types, and have a general form of:

    .. parsed-literal::
    
        |...| \*\*\*\*\*\*\*\*\*\*\*\*( |...|::message )\*\*\*\*\*\*\*\*\*\*\*\*)(types<t1, t2,... tn>) |...|


Example
-------

::
    
    template< typename T > struct my
    {
        // ...
        BOOST_MPL_ASSERT_MSG( 
              is_integral<T>::value
            , NON_INTEGRAL_TYPES_ARE_NOT_ALLOWED
            , (T)
            );
    };
    
    my<void*> test;

    // In instantiation of `my<void*>':
    //   instantiated from here
    // conversion from `
    //   mpl_::failed************(my<void*>::
    //   NON_INTEGRAL_TYPES_ARE_NOT_ALLOWED::************)(void*)
    //   ' to non-scalar type `mpl_::assert<false>' requested


See also
--------

|Asserts|, |BOOST_MPL_ASSERT|, |BOOST_MPL_ASSERT_NOT|, |BOOST_MPL_ASSERT_RELATION|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
