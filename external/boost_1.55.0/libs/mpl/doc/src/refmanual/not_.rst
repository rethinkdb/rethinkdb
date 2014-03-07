.. Metafunctions/Logical Operations//not_ |30

not\_
=====

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename F
        >
    struct not\_
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of *logical not* (``!``) operation on its argument.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/not.hpp>
    #include <boost/mpl/logical.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| ``F``         | Nullary |Metafunction|    | Operation's argument.                         |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

For arbitrary nullary |Metafunction| ``f``:

.. parsed-literal::

    typedef not_<f>::type r;

:Return type:
    |Integral Constant|.
 
:Semantics:
    Equivalent to 
    
    .. parsed-literal::
        
        typedef bool_< (!f::type::value) > r;

.. ..........................................................................

.. parsed-literal::

    typedef not_<f> r;

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        struct r : not_<f>::type {};


Example
-------

.. parsed-literal::
    
    BOOST_MPL_ASSERT_NOT(( not_< true\_ > ));
    BOOST_MPL_ASSERT(( not_< false\_ > ));


See also
--------

|Metafunctions|, |Logical Operations|, |and_|, |or_|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
