.. Macros/Configuration//BOOST_MPL_LIMIT_VECTOR_SIZE |30

BOOST_MPL_LIMIT_VECTOR_SIZE
===========================

Synopsis
--------

.. parsed-literal::

    #if !defined(BOOST_MPL_LIMIT_VECTOR_SIZE)
    #   define BOOST_MPL_LIMIT_VECTOR_SIZE \\
            |idic| \\
    /\*\*/
    #endif


Description
-----------

``BOOST_MPL_LIMIT_VECTOR_SIZE`` is an overridable configuration macro regulating
the maximum arity of the ``vector``\ 's and ``vector_c``\ 's |variadic forms|. In this 
implementation of the library, ``BOOST_MPL_LIMIT_VECTOR_SIZE`` has a default value
of 20. To override the default limit, define ``BOOST_MPL_LIMIT_VECTOR_SIZE`` to
the desired maximum arity rounded up to the nearest multiple of ten before 
including any library header. |preprocessed headers disclaimer|


Example
-------

.. parsed-literal::

    #define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
    #define BOOST_MPL_LIMIT_VECTOR_SIZE 10
    ``#``\ include <boost/mpl/vector.hpp>
    
    using namespace boost::mpl;

    typedef vector_c<int,1> v_1;
    typedef vector_c<int,1,2,3,4,5,6,7,8,9,10> v_10;
    // typedef vector_c<int,1,2,3,4,5,6,7,8,9,10,11> v_11; // error!


See also
--------

|Configuration|, |BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS|, |BOOST_MPL_LIMIT_LIST_SIZE|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
