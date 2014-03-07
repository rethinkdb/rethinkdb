.. Data Types/Numeric//integral_c |50

integral_c
==========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename T, T N
        >
    struct integral_c
    {
        // |unspecified|
        // ...
    };


Description
-----------

A generic |Integral Constant| wrapper.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/integral_c.hpp>


Model of
--------

|Integral Constant|


Parameters
----------

+---------------+-------------------------------+---------------------------+
| Parameter     | Requirement                   | Description               |
+===============+===============================+===========================+
| ``T``         | An integral type              | Wrapper's value type.     |
+---------------+-------------------------------+---------------------------+
| ``N``         | An integral constant          | A value to wrap.          | 
+---------------+-------------------------------+---------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Integral Constant|.

For arbitrary integral type ``t`` and integral constant ``n``:

+-----------------------+-----------------------------------------------------------+
| Expression            | Semantics                                                 |
+=======================+===========================================================+
| ``integral_c<t,c>``   | An |Integral Constant| ``x`` such that ``x::value == c``  |
|                       | and ``x::value_type`` is identical to ``t``.              |
+-----------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::

    typedef integral_c<short,8> eight;
    
    BOOST_MPL_ASSERT(( is_same< eight::value_type, short > ));
    BOOST_MPL_ASSERT(( is_same< eight::type, eight > ));
    BOOST_MPL_ASSERT(( is_same< next< eight >::type, integral_c<short,9> > ));
    BOOST_MPL_ASSERT(( is_same< prior< eight >::type, integral_c<short,7> > ));
    BOOST_MPL_ASSERT_RELATION( (eight::value), ==, 8 );
    assert( eight() == 8 );


See also
--------

|Data Types|, |Integral Constant|, |bool_|, |int_|, |long_|, |size_t|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
