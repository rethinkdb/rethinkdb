.. Macros/Introspection//BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF

BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF
=================================

Synopsis
--------

.. parsed-literal::

    #define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, default\_) \\
        |unspecified-token-seq| \\
    /\*\*/


Description
-----------

Expands into a definition of a boolean unary |Metafunction| ``trait`` 
such that for any type ``x`` ``trait<x>::value == true`` if and only
if ``x`` is a class type and has a nested type memeber ``x::name``.

On the deficient compilers not capabale of performing the detection, 
``trait<x>::value`` always returns a fallback value ``default_``. 
A boolean configuraion macro, |BOOST_MPL_CFG_NO_HAS_XXX|, is provided 
to signal or override the "deficient" status of a particular compiler.
|Note:| The fallback value can also be provided at the point of the 
metafunction invocation; see the `Expression semantics` section for 
details |-- end note|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/has_xxx.hpp>


Parameters
----------

+---------------+-------------------------------+---------------------------------------------------+
| Parameter     | Requirement                   | Description                                       |
+===============+===============================+===================================================+
| ``trait``     | A legal identifier token      | A name of the metafunction to be generated.       |
+---------------+-------------------------------+---------------------------------------------------+
| ``name``      | A legal identifier token      | A name of the member being detected.              |
+---------------+-------------------------------+---------------------------------------------------+
| ``default_``  | An boolean constant           | A fallback value for the deficient compilers.     |
+---------------+-------------------------------+---------------------------------------------------+


Expression semantics
--------------------

For any legal C++ identifiers ``trait`` and ``name``, boolean constant expression ``c1``, 
boolean |Integral Constant| ``c2``, and arbitrary type ``x``:

.. parsed-literal::

    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, c1)

:Precondition:
    Appears at namespace scope.

:Return type:
    None.

:Semantics:
    Expands into an equivalent of the following class template definition

    .. parsed-literal::

        template< typename X, typename fallback = boost::mpl::bool_<c1> >
        struct trait
        {
            // |unspecified|
            // ...
        };
        
    where ``trait`` is a boolean |Metafunction| with the following semantics:
    
    .. parsed-literal::

        typedef trait<x>::type r;

    :Return type:
        |Integral Constant|.

    :Semantics:
        If |BOOST_MPL_CFG_NO_HAS_XXX| is defined, ``r::value == c1``;
        otherwise, ``r::value == true`` if and only if ``x`` is a class type 
        that has a nested type memeber ``x::name``.
    
    
    .. parsed-literal::

        typedef trait< x,c2 >::type r;

    :Return type:
        |Integral Constant|.

    :Semantics:
        If |BOOST_MPL_CFG_NO_HAS_XXX| is defined, ``r::value == c2::value``;
        otherwise, equivalent to

        .. parsed-literal::

            typedef trait<x>::type r;
        

Example
-------

.. parsed-literal::
    
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_xxx, xxx, false)

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

|Macros|, |BOOST_MPL_HAS_XXX_TRAIT_DEF|, |BOOST_MPL_CFG_NO_HAS_XXX|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
