.. Macros/Configuration//BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS |10

BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
=====================================
.. _`BOOST_MPL_CFG_NO_PREPROCESSED`:

Synopsis
--------

.. parsed-literal::

    // #define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS


Description
-----------

``BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS`` is an boolean configuration macro 
regulating library's internal use of preprocessed headers. When defined, it
instructs the MPL to discard the pre-generated headers found in 
``boost/mpl/aux_/preprocessed`` directory and use `preprocessor 
metaprogramming`__ techniques to generate the necessary versions of the 
library components on the fly.

In this implementation of the library, the macro is not defined by default.
To change the default configuration, define 
``BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS`` before  including any library 
header. 

__ http://boost-consulting.com/tmpbook/preprocessor.html


See also
--------

|Macros|, |Configuration|

.. |preprocessed headers| replace:: `preprocessed headers`_
.. _`preprocessed headers`: `BOOST_MPL_CFG_NO_PREPROCESSED`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
