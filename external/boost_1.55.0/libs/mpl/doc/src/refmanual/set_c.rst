.. Sequences/Classes//set_c |90

set_c
=====

Description
-----------

``set_c`` is an |Integral Sequence Wrapper| for |set|. As such, it shares
all |set| characteristics and requirements, and differs only in the way the 
original sequence content is specified.

Header
------

+-------------------+-------------------------------------------------------+
| Sequence form     | Header                                                |
+===================+=======================================================+
| Variadic          | ``#include <boost/mpl/set_c.hpp>``                    |
+-------------------+-------------------------------------------------------+
| Numbered          | ``#include <boost/mpl/set/set``\ *n*\ ``_c.hpp>``     |
+-------------------+-------------------------------------------------------+


Model of
--------

* |Variadic Sequence|
* |Associative Sequence|
* |Extensible Associative Sequence|


Expression semantics
--------------------

|Semantics disclaimer...| |set|.

.. workaround substitution bug (should be replace:: set\ *n*\ _c<T,\ |c1...cn|>)
.. |setn_c<T,...>| replace:: set\ *n*\ _c<T,\ *c*\ :sub:`1`,\ *c*\ :sub:`2`,... \ *c*\ :sub:`n`\ >

+---------------------------------------+-----------------------------------------------+
| Expression                            | Semantics                                     |
+=======================================+===============================================+
| .. parsed-literal::                   | A |set| of integral constant wrappers         |
|                                       | ``integral_c<T,``\ |c1|\ ``>``,               |
|    set_c<T,\ |c1...cn|>               | ``integral_c<T,``\ |c2|\ ``>``, ...           |
|    |setn_c<T,...>|                    | ``integral_c<T,``\ |cn|\ ``>``;               |
|                                       | see |Integral Sequence Wrapper|.              |
+---------------------------------------+-----------------------------------------------+
| .. parsed-literal::                   | Identical to ``set``\ *n*\ ``<``              |
|                                       | ``integral_c<T,``\ |c1|\ ``>``,               |
|    set_c<T,\ |c1...cn|>::type         | ``integral_c<T,``\ |c2|\ ``>``, ...           |
|    |setn_c<T,...>|::type              | ``integral_c<T,``\ |cn|\ ``>`` ``>``;         |
|                                       | see |Integral Sequence Wrapper|.              |
+---------------------------------------+-----------------------------------------------+
| .. parsed-literal::                   | Identical to ``T``; see                       |
|                                       | |Integral Sequence Wrapper|.                  |
|   set_c<T,\ |c1...cn|>::value_type    |                                               |
|   |setn_c<T,...>|::value_type         |                                               |
+---------------------------------------+-----------------------------------------------+


Example
-------

.. parsed-literal::

    typedef set_c< int,1,3,5,7,9 > odds;

    BOOST_MPL_ASSERT_RELATION( size<odds>::value, ==, 5 );
    BOOST_MPL_ASSERT_NOT(( empty<odds> ));
    
    BOOST_MPL_ASSERT(( has_key< odds, integral_c<int,5> > ));
    BOOST_MPL_ASSERT_NOT(( has_key< odds, integral_c<int,4> > ));
    BOOST_MPL_ASSERT_NOT(( has_key< odds, integral_c<int,15> > ));


See also
--------

|Sequences|, |Integral Sequence Wrapper|, |set|, |integral_c|, |vector_c|, |list_c|, |range_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
