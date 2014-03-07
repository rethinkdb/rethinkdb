.. Metafunctions/Type Selection//eval_if |30

eval_if
=======

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename C
        , typename F1
        , typename F2
        >
    struct eval_if
    {
        typedef |unspecified| type;
    };



Description
-----------

Evaluates one of its two |nullary metafunction| arguments, ``F1`` or ``F2``, depending 
on the value ``C``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/eval_if.hpp>


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``C``         | |Integral Constant|               | An evaluation condition.                      |
+---------------+-----------------------------------+-----------------------------------------------+
| ``F1``, ``F2``| Nullary |Metafunction|            | Metafunctions to select for evaluation from.  |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any |Integral Constant| ``c`` and nullary |Metafunction|\ s ``f1``, ``f2``:


.. parsed-literal::

    typedef eval_if<c,f1,f2>::type t;

:Return type:
    Any type.

:Semantics:
    If ``c::value == true``, ``t`` is identical to ``f1::type``; otherwise ``t`` is 
    identical to ``f2::type``.


Example
-------

.. parsed-literal::
    
    typedef eval_if< true\_, identity<char>, identity<long> >::type t1;
    typedef eval_if< false\_, identity<char>, identity<long> >::type t2;

    BOOST_MPL_ASSERT(( is_same<t1,char> ));
    BOOST_MPL_ASSERT(( is_same<t2,long> ));


See also
--------

|Metafunctions|, |Integral Constant|, |eval_if_c|, |if_|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
