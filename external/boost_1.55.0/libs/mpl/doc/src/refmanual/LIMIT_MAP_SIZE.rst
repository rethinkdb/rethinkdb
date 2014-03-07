.. Macros/Configuration//BOOST_MPL_LIMIT_MAP_SIZE |60

BOOST_MPL_LIMIT_MAP_SIZE
========================

Synopsis
--------

.. parsed-literal::

    #if !defined(BOOST_MPL_LIMIT_MAP_SIZE)
    #   define BOOST_MPL_LIMIT_MAP_SIZE \\
            |idic| \\
    /\*\*/
    #endif


Description
-----------

``BOOST_MPL_LIMIT_MAP_SIZE`` is an overridable configuration macro regulating
the maximum arity of the ``map``\ 's `variadic form`__. In this 
implementation of the library, ``BOOST_MPL_LIMIT_MAP_SIZE`` has a default value
of 20. To override the default limit, define ``BOOST_MPL_LIMIT_MAP_SIZE`` to
the desired maximum arity rounded up to the nearest multiple of ten before 
including any library header. |preprocessed headers disclaimer|

__ `Variadic Sequence`_


Example
-------

.. parsed-literal::

    #define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
    #define BOOST_MPL_LIMIT_MAP_SIZE 10
    ``#``\ include <boost/mpl/map.hpp>
    ``#``\ include <boost/mpl/pair.hpp>
    ``#``\ include <boost/mpl/int.hpp>
    
    using namespace boost::mpl;

    template< int i > struct ints : pair< int_<i>,int_<i> > {};
    
    typedef map< ints<1> > m_1;
    typedef map< ints<1>, ints<2>, ints<3>, ints<4>, ints<5>
        ints<6>, ints<7>, ints<8>, ints<9>, ints<10> > m_10;
    
    // typedef map< ints<1>, ints<2>, ints<3>, ints<4>, ints<5>
    //     ints<6>, ints<7>, ints<8>, ints<9>, ints<10>, ints<11> > m_11; // error!


See also
--------

|Configuration|, |BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS|, |BOOST_MPL_LIMIT_SET_SIZE|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
