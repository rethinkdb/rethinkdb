.. Macros/Configuration//BOOST_MPL_LIMIT_STRING_SIZE |65

BOOST_MPL_LIMIT_STRING_SIZE
===========================

Synopsis
--------

.. parsed-literal::

    #if !defined(BOOST_MPL_LIMIT_STRING_SIZE)
    #   define BOOST_MPL_LIMIT_STRING_SIZE \\
            |idic| \\
    /\*\*/
    #endif


Description
-----------

``BOOST_MPL_LIMIT_STRING_SIZE`` is an overridable configuration macro regulating
the maximum arity of the ``string``\ 's |variadic forms|. In this 
implementation of the library, ``BOOST_MPL_LIMIT_STRING_SIZE`` has a default value
of 32. To override the default limit, define ``BOOST_MPL_LIMIT_STRING_SIZE`` to
the desired maximum arity before including any library header.


Example
-------

.. parsed-literal::

    #define BOOST_MPL_LIMIT_STRING_SIZE 8
    ``#``\ include <boost/mpl/string.hpp>
    
    using namespace boost::mpl;

    typedef string<'a'> s_1;
    typedef string<'abcd','efgh'> s_8;
    // typedef string<'abcd','efgh','i'> s_9; // error!


See also
--------

|Configuration|, |BOOST_MPL_LIMIT_VECTOR_SIZE|


.. copyright:: Copyright ©  2009 Eric Niebler
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
