.. Sequences/Classes//vector_c |70

vector_c
========

Description
-----------

``vector_c`` is an |Integral Sequence Wrapper| for |vector|. As such, it shares
all |vector| characteristics and requirements, and differs only in the way the 
original sequence content is specified.

Header
------

+-------------------+-----------------------------------------------------------+
| Sequence form     | Header                                                    |
+===================+===========================================================+
| Variadic          | ``#include <boost/mpl/vector_c.hpp>``                     |
+-------------------+-----------------------------------------------------------+
| Numbered          | ``#include <boost/mpl/vector/vector``\ *n*\ ``_c.hpp>``   |
+-------------------+-----------------------------------------------------------+


Model of
--------

* |Integral Sequence Wrapper|
* |Variadic Sequence|
* |Random Access Sequence|
* |Extensible Sequence|
* |Back Extensible Sequence|
* |Front Extensible Sequence|


Expression semantics
--------------------

|Semantics disclaimer...| |vector|.

.. workaround substitution bug (should be replace:: vector\ *n*\ _c<T,\ |c1...cn|>)
.. |vectorn_c<T,...>| replace:: vector\ *n*\ _c<T,\ *c*\ :sub:`1`,\ *c*\ :sub:`2`,... \ *c*\ :sub:`n`\ >

+-------------------------------------------+-----------------------------------------------+
| Expression                                | Semantics                                     |
+===========================================+===============================================+
| .. parsed-literal::                       | A |vector| of integral constant wrappers      |
|                                           | ``integral_c<T,``\ |c1|\ ``>``,               |
|    vector_c<T,\ |c1...cn|>                | ``integral_c<T,``\ |c2|\ ``>``, ...           |
|    |vectorn_c<T,...>|                     | ``integral_c<T,``\ |cn|\ ``>``;               |
|                                           | see |Integral Sequence Wrapper|.              |
+-------------------------------------------+-----------------------------------------------+
| .. parsed-literal::                       | Identical to ``vector``\ *n*\ ``<``           |
|                                           | ``integral_c<T,``\ |c1|\ ``>``,               |
|    vector_c<T,\ |c1...cn|>::type          | ``integral_c<T,``\ |c2|\ ``>``, ...           |
|    |vectorn_c<T,...>|::type               | ``integral_c<T,``\ |cn|\ ``>`` ``>``;         |
|                                           | see |Integral Sequence Wrapper|.              |
+-------------------------------------------+-----------------------------------------------+
| .. parsed-literal::                       | Identical to ``T``; see                       |
|                                           | |Integral Sequence Wrapper|.                  |
|   vector_c<T,\ |c1...cn|>::value_type     |                                               |
|   |vectorn_c<T,...>|::value_type          |                                               |
+-------------------------------------------+-----------------------------------------------+


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,1,1,2,3,5,8,13,21,34> fibonacci;
    typedef push_back<fibonacci,int_<55> >::type fibonacci2;

    BOOST_MPL_ASSERT_RELATION( front<fibonacci2>::type::value, ==, 1 );
    BOOST_MPL_ASSERT_RELATION( back<fibonacci2>::type::value, ==, 55 );


See also
--------

|Sequences|, |Integral Sequence Wrapper|, |vector|, |integral_c|, |set_c|, |list_c|, |range_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
