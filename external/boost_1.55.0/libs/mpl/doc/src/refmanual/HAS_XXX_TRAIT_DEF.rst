.. Macros/Introspection//BOOST_MPL_HAS_XXX_TRAIT_DEF

BOOST_MPL_HAS_XXX_TRAIT_DEF
===========================

Synopsis
--------

.. parsed-literal::

    #define BOOST_MPL_HAS_XXX_TRAIT_DEF(name) \\
        |unspecified-token-seq| \\
    /\*\*/


Description
-----------

Expands into a definition of a boolean unary |Metafunction| ``has_name`` 
such that for any type ``x`` ``has_name<x>::value == true`` if and only
if ``x`` is a class type and has a nested type memeber ``x::name``.

On the deficient compilers not capabale of performing the detection, 
``has_name<x>::value`` always returns ``false``. A boolean configuraion 
macro, |BOOST_MPL_CFG_NO_HAS_XXX|, is provided to signal or override 
the "deficient" status of a particular compiler.

|Note:| |BOOST_MPL_HAS_XXX_TRAIT_DEF| is a simplified front end to
the |BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF| introspection macro |-- end note|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/has_xxx.hpp>


Parameters
----------


+---------------+-------------------------------+---------------------------------------------------+
| Parameter     | Requirement                   | Description                                       |
+===============+===============================+===================================================+
| ``name``      | A legal identifier token      | A name of the member being detected.              |
+---------------+-------------------------------+---------------------------------------------------+


Expression semantics
--------------------

For any legal C++ identifier ``name``:

.. parsed-literal::

    BOOST_MPL_HAS_XXX_TRAIT_DEF(name)

:Precondition:
    Appears at namespace scope.

:Return type:
    None.

:Semantics:
    Equivalent to

    .. parsed-literal::

        BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(
              BOOST_PP_CAT(has\_,name), name, false
            )


Example
-------

.. parsed-literal::
    
    BOOST_MPL_HAS_XXX_TRAIT_DEF(xxx)

    struct test1 {};
    struct test2 { void xxx(); };
    struct test3 { int xxx; };
    struct test4 { static int xxx(); };
    struct test5 { template< typename T > struct xxx {}; };
    struct test6 { typedef int xxx; };
    struct test7 { struct xxx; };
    struct test8 { typedef void (\*xxx)(); };
    struct test9 { typedef void (xxx)(); };

    BOOST_MPL_ASSERT_NOT(( has_xxx<test1> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test2> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test3> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test4> ));
    BOOST_MPL_ASSERT_NOT(( has_xxx<test5> ));

    #if !defined(BOOST_MPL_CFG_NO_HAS_XXX)
    BOOST_MPL_ASSERT(( has_xxx<test6> ));
    BOOST_MPL_ASSERT(( has_xxx<test7> ));
    BOOST_MPL_ASSERT(( has_xxx<test8> ));
    BOOST_MPL_ASSERT(( has_xxx<test9> ));
    #endif
    
    BOOST_MPL_ASSERT(( has_xxx<test6,true\_> ));
    BOOST_MPL_ASSERT(( has_xxx<test7,true\_> ));
    BOOST_MPL_ASSERT(( has_xxx<test8,true\_> ));
    BOOST_MPL_ASSERT(( has_xxx<test9,true\_> ));


See also
--------

|Macros|, |BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF|, |BOOST_MPL_CFG_NO_HAS_XXX|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
