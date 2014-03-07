.. Macros/Introspection//BOOST_MPL_HAS_XXX_TEMPLATE_DEF

.. Copyright Daniel Walker 2007.
.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

BOOST_MPL_HAS_XXX_TEMPLATE_DEF
==============================

Synopsis
--------

.. parsed-literal::

    #define BOOST_MPL_HAS_XXX_TEMPLATE_DEF(name) \\
        |unspecified-token-seq| \\
    /\*\*/


Description
-----------

Expands into the definition of a boolean |Metafunction| ``has_name``
such that for any type ``x`` ``has_name<x>::value == true`` if and
only if ``x`` is a class type and has a nested template member
``x::template name`` with no more than
|BOOST_MPL_LIMIT_METAFUNCTION_ARITY| parameters.

On deficient compilers not capable of performing the detection,
``has_name<x>::value`` is always ``false``. A boolean configuration
macro, |BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE|, is provided to signal or
override the "deficient" status of a particular compiler.

|Note:| |BOOST_MPL_HAS_XXX_TEMPLATE_DEF| is a simplified front end to
the |BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF| introspection macro |-- end
note|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/has_xxx.hpp>


Parameters
----------


+---------------+-------------------------------+---------------------------------------------------+
| Parameter     | Requirement                   | Description                                       |
+===============+===============================+===================================================+
| ``name``      | A legal identifier token      | A name of the template member being detected.     |
+---------------+-------------------------------+---------------------------------------------------+


Expression semantics
--------------------

For any legal C++ identifier ``name``:

.. parsed-literal::

    BOOST_MPL_HAS_XXX_TEMPLATE_DEF(name)

:Precondition:
    Appears at namespace scope.

:Return type:
    None.

:Semantics:
    Equivalent to

    .. parsed-literal::

        BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF(
              BOOST_PP_CAT(has\_,name), name, false
            )


Example
-------

.. parsed-literal::
    
    BOOST_MPL_HAS_XXX_TEMPLATE_DEF(xxx)
    
    struct test1  {};
    struct test2  { void xxx(); };
    struct test3  { int xxx; };
    struct test4  { static int xxx(); };
    struct test5  { typedef int xxx; };
    struct test6  { struct xxx; };
    struct test7  { typedef void (\*xxx)(); };
    struct test8  { typedef void (xxx)(); };
    struct test9  { template< class T > struct xxx {}; };
    
    BOOST_MPL_ASSERT_NOT(( has_xxx<test1> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test2> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test3> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test4> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test5> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test6> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test7> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test8> ));
    
    #if !defined(BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE)
    BOOST_MPL_ASSERT(( has_xxx<test9> ));
    #endif
    
    BOOST_MPL_ASSERT(( has_xxx<test9, true\_> ));


See also
--------

|Macros|, |BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF|,
|BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE|, |BOOST_MPL_LIMIT_METAFUNCTION_ARITY|

