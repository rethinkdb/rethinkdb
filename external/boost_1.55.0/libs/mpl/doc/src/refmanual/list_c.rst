.. Sequences/Classes//list_c |80

list_c
======

Description
-----------

``list_c`` is an |Integral Sequence Wrapper| for |list|. As such, it shares
all |list| characteristics and requirements, and differs only in the way the 
original sequence content is specified.

Header
------

+-------------------+-------------------------------------------------------+
| Sequence form     | Header                                                |
+===================+=======================================================+
| Variadic          | ``#include <boost/mpl/list_c.hpp>``                   |
+-------------------+-------------------------------------------------------+
| Numbered          | ``#include <boost/mpl/list/list``\ *n*\ ``_c.hpp>``   |
+-------------------+-------------------------------------------------------+


Model of
--------

* |Integral Sequence Wrapper|
* |Variadic Sequence|
* |Forward Sequence|
* |Extensible Sequence|
* |Front Extensible Sequence|


Expression semantics
--------------------

|Semantics disclaimer...| |list|.

.. workaround substitution bug (should be replace:: list\ *n*\ _c<T,\ |c1...cn|>)
.. |listn_c<T,...>| replace:: list\ *n*\ _c<T,\ *c*\ :sub:`1`,\ *c*\ :sub:`2`,... \ *c*\ :sub:`n`\ >

+---------------------------------------+-----------------------------------------------+
| Expression                            | Semantics                                     |
+=======================================+===============================================+
| .. parsed-literal::                   | A |list| of integral constant wrappers        |
|                                       | ``integral_c<T,``\ |c1|\ ``>``,               |
|    list_c<T,\ |c1...cn|>              | ``integral_c<T,``\ |c2|\ ``>``, ...           |
|    |listn_c<T,...>|                   | ``integral_c<T,``\ |cn|\ ``>``;               |
|                                       | see |Integral Sequence Wrapper|.              |
+---------------------------------------+-----------------------------------------------+
| .. parsed-literal::                   | Identical to ``list``\ *n*\ ``<``             |
|                                       | ``integral_c<T,``\ |c1|\ ``>``,               |
|    list_c<T,\ |c1...cn|>::type        | ``integral_c<T,``\ |c2|\ ``>``, ...           |
|    |listn_c<T,...>|::type             | ``integral_c<T,``\ |cn|\ ``>`` ``>``;         |
|                                       | see |Integral Sequence Wrapper|.              |
+---------------------------------------+-----------------------------------------------+
| .. parsed-literal::                   | Identical to ``T``; see                       |
|                                       | |Integral Sequence Wrapper|.                  |
|   list_c<T,\ |c1...cn|>::value_type   |                                               |
|   |listn_c<T,...>|::value_type        |                                               |
+---------------------------------------+-----------------------------------------------+


Example
-------

.. parsed-literal::
    
    typedef list_c<int,1,2,3,5,7,12,19,31> fibonacci;
    typedef push_front<fibonacci,int_<1> >::type fibonacci2;
    
    BOOST_MPL_ASSERT_RELATION( front<fibonacci2>::type::value, ==, 1 );


See also
--------

|Sequences|, |Integral Sequence Wrapper|, |list|, |integral_c|, |vector_c|, |set_c|, |range_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
