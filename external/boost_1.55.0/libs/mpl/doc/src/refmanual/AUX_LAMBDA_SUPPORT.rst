.. Macros/Broken Compiler Workarounds//BOOST_MPL_AUX_LAMBDA_SUPPORT

BOOST_MPL_AUX_LAMBDA_SUPPORT
============================

Synopsis
--------

.. parsed-literal::
    
    #define BOOST_MPL_AUX_LAMBDA_SUPPORT(arity, fun, params) \\
        |unspecified-token-seq| \\
    /\*\*/



Description
-----------

Enables metafunction ``fun`` for the use in |Lambda Expression|\ s on 
compilers that don't support partial template specialization or/and 
template template parameters. Expands to nothing on conforming compilers.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/aux\_/lambda_support.hpp>


Parameters
----------

+---------------+-------------------------------+---------------------------------------------------+
| Parameter     | Requirement                   | Description                                       |
+===============+===============================+===================================================+
| ``arity``     | An integral constant          | The metafunction's arity, i.e. the number of its  |
|               |                               | template parameters, including the defaults.      |
+---------------+-------------------------------+---------------------------------------------------+
| ``fun``       | A legal identifier token      | The metafunction's name.                          |
+---------------+-------------------------------+---------------------------------------------------+
| ``params``    | A |PP-tuple|                  | A tuple of the metafunction's parameter names, in |
|               |                               | their original order, including the defaults.     |
+---------------+-------------------------------+---------------------------------------------------+


Expression semantics
--------------------

For any integral constant ``n``, a |Metafunction| ``fun``, and arbitrary types |A1...An|:


.. parsed-literal::

    template< typename A1,\ |...| typename A\ *n* > struct fun
    {
        // |...|
    
        BOOST_MPL_AUX_LAMBDA_SUPPORT(n, fun, (A1,\ |...|\ A\ *n*\ ))
    };

:Precondition:
    Appears in ``fun``\ 's scope, immediately followed by the scope-closing 
    bracket (``}``).

:Return type:
    None.

:Semantics:
    Expands to nothing and has no effect on conforming compilers. On compilers that 
    don't support partial template specialization or/and template template parameters
    expands to an unspecified token sequence enabling ``fun`` to participate in
    |Lambda Expression|\ s with the semantics described in this manual.


Example
-------

.. parsed-literal::
    
    template< typename T, typename U = int > struct f
    {
        typedef T type[sizeof(U)];
    
        BOOST_MPL_AUX_LAMBDA_SUPPORT(2, f, (T,U))
    };
    
    typedef apply\ ``1``\< f<char,_1>,long >::type r;
    BOOST_MPL_ASSERT(( is_same< r, char[sizeof(long)] > ));


See also
--------

|Macros|, |Metafunctions|, |Lambda Expression|


.. |PP-tuple| replace:: `PP-tuple <http://www.boost.org/libs/preprocessor/doc/data/tuples.html>`__


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
