.. Metafunctions/Type Selection//eval_if_c |40

eval_if_c
=========

Synopsis
--------

.. parsed-literal::
    
    template< 
          bool c
        , typename F1
        , typename F2
        >
    struct eval_if_c
    {
        typedef |unspecified| type;
    };



Description
-----------

Evaluates one of its two |nullary metafunction| arguments, ``F1`` or ``F2``, depending 
on the value of integral constant ``c``. ``eval_if_c<c,f1,f2>::type`` is a shorcut 
notation for ``eval_if< bool_<c>,f1,f2 >::type``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/eval_if.hpp>


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``c``         | An integral constant              | An evaluation condition.                      |
+---------------+-----------------------------------+-----------------------------------------------+
| ``F1``, ``F2``| Nullary |Metafunction|            | Metafunctions to select for evaluation from.  |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

For any integral constant ``c`` and nullary |Metafunction|\ s ``f1``, ``f2``:


.. parsed-literal::

    typedef eval_if_c<c,f1,f2>::type t;

:Return type:
    Any type.

:Semantics:
    Equivalent to ``typedef eval_if< bool_<c>,f1,f2 >::type t;`` 


Example
-------

.. parsed-literal::
    
    typedef eval_if_c< true, identity<char>, identity<long> >::type t1;
    typedef eval_if_c< false, identity<char>, identity<long> >::type t2;

    BOOST_MPL_ASSERT(( is_same<t1,char> ));
    BOOST_MPL_ASSERT(( is_same<t2,long> ));


See also
--------

|Metafunctions|, |Integral Constant|, |eval_if|, |if_|, |bool_|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
