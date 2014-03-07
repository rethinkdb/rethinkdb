.. Data Types/Numeric//char_ |60

char\_
======

Synopsis
--------

.. parsed-literal::
    
    template<
          char N
        >
    struct char\_
    {
        // |unspecified|
        // ...
    };


Description
-----------

An |Integral Constant| wrapper for ``char``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/char.hpp>


Model of
--------

|Integral Constant|


Parameters
----------

+---------------+-------------------------------+---------------------------+
| Parameter     | Requirement                   | Description               |
+===============+===============================+===========================+
| ``N``         | A character constant          | A value to wrap.          | 
+---------------+-------------------------------+---------------------------+

Expression semantics
--------------------

|Semantics disclaimer...| |Integral Constant|.

For arbitrary character constant ``c``:

+-------------------+-----------------------------------------------------------+
| Expression        | Semantics                                                 |
+===================+===========================================================+
| ``char_<c>``      | An |Integral Constant| ``x`` such that ``x::value == c``  |
|                   | and ``x::value_type`` is identical to ``char``.           |
+-------------------+-----------------------------------------------------------+


Example
-------

.. parsed-literal::

    typedef char_<'c'> c;
    
    BOOST_MPL_ASSERT(( is_same< c::value_type, char > ));
    BOOST_MPL_ASSERT(( is_same< c::type, c > ));
    BOOST_MPL_ASSERT(( is_same< next< c >::type, char_<'d'> > ));
    BOOST_MPL_ASSERT(( is_same< prior< c >::type, char_<'b'> > ));
    BOOST_MPL_ASSERT_RELATION( (c::value), ==, 'c' );
    assert( c() == 'c' );


See also
--------

|Data Types|, |Integral Constant|, |int_|, |size_t|, |integral_c|


.. copyright:: Copyright ©  2009 Eric Niebler
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
