.. Macros/Configuration//BOOST_MPL_LIMIT_UNROLLING |70

BOOST_MPL_LIMIT_UNROLLING
=========================

Synopsis
--------

.. parsed-literal::

    #if !defined(BOOST_MPL_LIMIT_UNROLLING)
    #   define BOOST_MPL_LIMIT_UNROLLING \\
            |idic| \\
    /\*\*/
    #endif


Description
-----------

``BOOST_MPL_LIMIT_UNROLLING`` is an overridable configuration macro regulating
the unrolling depth of the library's iteration algorithms. In this implementation 
of the library, ``BOOST_MPL_LIMIT_UNROLLING`` has a default value of 4. To 
override the default, define ``BOOST_MPL_LIMIT_UNROLLING`` to the desired 
value before including any library header. 
|preprocessed headers disclaimer|


Example
-------

Except for overall library performace, overriding the 
``BOOST_MPL_LIMIT_UNROLLING``\ 's default value has no user-observable effects.

See also
--------

|Configuration|, |BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
