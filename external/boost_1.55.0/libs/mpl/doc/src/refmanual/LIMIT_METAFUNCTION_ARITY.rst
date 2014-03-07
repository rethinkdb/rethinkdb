.. Macros/Configuration//BOOST_MPL_LIMIT_METAFUNCTION_ARITY |20

BOOST_MPL_LIMIT_METAFUNCTION_ARITY
==================================

Synopsis
--------

.. parsed-literal::

    #if !defined(BOOST_MPL_LIMIT_METAFUNCTION_ARITY)
    #   define BOOST_MPL_LIMIT_METAFUNCTION_ARITY \\
            |idic| \\
    /\*\*/
    #endif


Description
-----------

``BOOST_MPL_LIMIT_METAFUNCTION_ARITY`` is an overridable configuration macro 
regulating the maximum supported arity of `metafunctions`__ and 
`metafunction classes`__. In this implementation of the 
library, ``BOOST_MPL_LIMIT_METAFUNCTION_ARITY`` has a default value of 5. To 
override the default limit, define ``BOOST_MPL_LIMIT_METAFUNCTION_ARITY`` to
the desired maximum arity before including any library header. 
|preprocessed headers disclaimer|

__ `Metafunction`_
__ `Metafunction Class`_


Example
-------

.. parsed-literal::

    #define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
    #define BOOST_MPL_LIMIT_METAFUNCTION_ARITY 2
    ``#``\ include <boost/mpl/apply.hpp>
    
    using namespace boost::mpl;

    template< typename T1, typename T2 > struct second
    {
        typedef T2 type;
    };

    template< typename T1, typename T2, typename T3 > struct third
    {
        typedef T3 type;
    };

    typedef apply< second<_1,_2_>,int,long >::type r1;
    // typedef apply< third<_1,_2_,_3>,int,long,float >::type r2; // error!


See also
--------

|Macros|, |Configuration|, |BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
