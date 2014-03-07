.. Data Types/Numeric//size_t |40

size_t
======

Synopsis
--------

.. parsed-literal::
    
    template<
          std::size_t N
        >
    struct size_t
    {
        // |unspecified|
        // ...
    };


Description
-----------

An |Integral Constant| wrapper for ``std::size_t``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/size_t.hpp>


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
| ``size_t<c>``     | An |Integral Constant| ``x`` such that ``x::value == c``  |
|                   | and ``x::value_type`` is identical to ``std::size_t``.    |
+-------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::

    typedef size_t<8> eight;
    
    BOOST_MPL_ASSERT(( is_same< eight::value_type, std::size_t > ));
    BOOST_MPL_ASSERT(( is_same< eight::type, eight > ));
    BOOST_MPL_ASSERT(( is_same< next< eight >::type, size_t<9> > ));
    BOOST_MPL_ASSERT(( is_same< prior< eight >::type, size_t<7> > ));
    BOOST_MPL_ASSERT_RELATION( (eight::value), ==, 8 );
    assert( eight() == 8 );


See also
--------

|Data Types|, |Integral Constant|, |int_|, |long_|, |integral_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
