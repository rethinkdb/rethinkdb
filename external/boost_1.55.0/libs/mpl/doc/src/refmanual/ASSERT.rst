.. Macros/Asserts//BOOST_MPL_ASSERT

BOOST_MPL_ASSERT
================

Synopsis
--------

.. parsed-literal::
    
    #define BOOST_MPL_ASSERT( pred ) \\
        |unspecified-token-seq| \\
    /\*\*/


Description
-----------

Generates a compilation error when the predicate ``pred`` holds false.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/assert.hpp>


Parameters
----------

+---------------+-----------------------------------+-------------------------------+
| Parameter     | Requirement                       | Description                   |
+===============+===================================+===============================+
| ``pred``      | Boolean nullary |Metafunction|    | A predicate to be asserted.   |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

For any boolean nullary |Metafunction| ``pred``:


.. parsed-literal::

    BOOST_MPL_ASSERT(( pred ));

:Return type:
    None.

:Semantics:
    Generates a compilation error if ``pred::type::value != true``, otherwise
    has no effect. Note that double parentheses are required even if no commas 
    appear in the condition. 
    
    When possible within the compiler's diagnostic capabilities,
    the error message will include the predicate's full type name, and have a 
    general form of:

    .. parsed-literal::
    
       |...| \*\*\*\*\*\*\*\*\*\*\*\* pred::\*\*\*\*\*\*\*\*\*\*\*\* |...|


Example
-------

::
    
    template< typename T, typename U > struct my
    {
        // ...
        BOOST_MPL_ASSERT(( is_same< T,U > ));
    };
    
    my<void*,char*> test;

    // In instantiation of `my<void, char*>':
    //   instantiated from here
    // conversion from `
    //   mpl_::failed************boost::is_same<void, char*>::************' to 
    //   non-scalar type `mpl_::assert<false>' requested


See also
--------

|Asserts|, |BOOST_MPL_ASSERT_NOT|, |BOOST_MPL_ASSERT_MSG|, |BOOST_MPL_ASSERT_RELATION|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
