.. Data Types/Numeric//int_ |20

int\_
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          int N
        >
    struct int\_
    {
        // |unspecified|
        // ...
    };


Description
-----------

An |Integral Constant| wrapper for ``int``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/int.hpp>


Model of
--------

|Integral Constant|


Parameters
----------

+---------------+-------------------------------+---------------------------+
| Parameter     | Requirement                   | Description               |
+===============+===============================+===========================+
| ``N``         | An integral constant          | A value to wrap.          | 
+---------------+-------------------------------+---------------------------+

Expression semantics
--------------------

|Semantics disclaimer...| |Integral Constant|.

For arbitrary integral constant ``n``:

+-------------------+-----------------------------------------------------------+
| Expression        | Semantics                                                 |
+===================+===========================================================+
| ``int_<c>``       | An |Integral Constant| ``x`` such that ``x::value == c``  |
|                   | and ``x::value_type`` is identical to ``int``.            |
+-------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::

    typedef int_<8> eight;
    
    BOOST_MPL_ASSERT(( is_same< eight::value_type, int > ));
    BOOST_MPL_ASSERT(( is_same< eight::type, eight > ));
    BOOST_MPL_ASSERT(( is_same< next< eight >::type, int_<9> > ));
    BOOST_MPL_ASSERT(( is_same< prior< eight >::type, int_<7> > ));
    BOOST_MPL_ASSERT_RELATION( (eight::value), ==, 8 );
    assert( eight() == 8 );


See also
--------

|Data Types|, |Integral Constant|, |long_|, |size_t|, |integral_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
