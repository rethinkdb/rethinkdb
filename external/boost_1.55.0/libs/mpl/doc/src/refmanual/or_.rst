.. Metafunctions/Logical Operations//or_ |20

or\_
====

Synopsis
--------

.. parsed-literal::
    
    template< 
          typename F1
        , typename F2
        |...|
        , typename F\ *n* = |unspecified|
        >
    struct or\_
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the result of short-circuit *logical or* (``||``) operation on its arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/or.hpp>
    #include <boost/mpl/logical.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| |F1...Fn|     | Nullary |Metafunction|    | Operation's arguments.                        |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

For arbitrary nullary |Metafunction|\ s |f1...fn|:

.. parsed-literal::

    typedef or_<f1,f2,\ |...|\ ,f\ *n*\>::type r;

:Return type:
    |Integral Constant|.

:Semantics:
    ``r`` is ``true_`` if either of ``f1::type::value``, ``f2::type::value``,... 
    ``fn::type::value`` expressions evaluates to ``true``, and ``false_`` otherwise; 
    guarantees left-to-right evaluation; the operands subsequent to the first 
    ``f``\ *i* metafunction that evaluates to ``true`` are not evaluated. 

.. ..........................................................................

.. parsed-literal::

    typedef or_<f1,f2,\ |...|\ ,f\ *n*\> r;

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        struct r : or_<f1,f2,\ |...|\ ,f\ *n*\>::type {};


Example
-------

.. parsed-literal::
    
    struct unknown;

    BOOST_MPL_ASSERT(( or_< true\_,true\_ > ));
    BOOST_MPL_ASSERT(( or_< false\_,true\_ > ));
    BOOST_MPL_ASSERT(( or_< true\_,false\_ > ));
    BOOST_MPL_ASSERT_NOT(( or_< false\_,false\_ > ));
    BOOST_MPL_ASSERT(( or_< true\_,unknown > )); // OK
    BOOST_MPL_ASSERT(( or_< true\_,unknown,unknown > )); // OK too


See also
--------

|Metafunctions|, |Logical Operations|, |and_|, |not_|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
