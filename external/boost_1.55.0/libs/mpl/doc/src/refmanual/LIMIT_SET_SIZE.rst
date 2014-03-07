.. Macros/Configuration//BOOST_MPL_LIMIT_SET_SIZE |50

BOOST_MPL_LIMIT_SET_SIZE
========================

Synopsis
--------

.. parsed-literal::

    #if !defined(BOOST_MPL_LIMIT_SET_SIZE)
    #   define BOOST_MPL_LIMIT_SET_SIZE \\
            |idic| \\
    /\*\*/
    #endif


Description
-----------

``BOOST_MPL_LIMIT_SET_SIZE`` is an overridable configuration macro regulating
the maximum arity of the ``set``\ 's and ``set_c``\ 's |variadic forms|. In this 
implementation of the library, ``BOOST_MPL_LIMIT_SET_SIZE`` has a default value
of 20. To override the default limit, define ``BOOST_MPL_LIMIT_SET_SIZE`` to
the desired maximum arity rounded up to the nearest multiple of ten before 
including any library header. |preprocessed headers disclaimer|


Example
-------

.. parsed-literal::

    #define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
    #define BOOST_MPL_LIMIT_SET_SIZE 10
    ``#``\ include <boost/mpl/set.hpp>
    
    using namespace boost::mpl;

    typedef set_c<int,1> s_1;
    typedef set_c<int,1,2,3,4,5,6,7,8,9,10> s_10;
    // typedef set_c<int,1,2,3,4,5,6,7,8,9,10,11> s_11; // error!


See also
--------

|Configuration|, |BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS|, |BOOST_MPL_LIMIT_MAP_SIZE|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
