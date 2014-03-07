.. Metafunctions/Composition and Argument Binding//_1,_2,..._n |10

Placeholders
============
.. _`placeholder`:


Synopsis
--------

.. parsed-literal::
    
    namespace placeholders {
    typedef |unspecified| _;
    typedef arg<1>      _1;
    typedef arg<2>      _2;
    |...|
    typedef arg<\ *n*\ >      _\ *n*\ ;
    }
    
    using placeholders::_;
    using placeholders::_1;
    using placeholders::_2;
    |...|
    using placeholders::_\ *n*\ ;
    

Description
-----------

A placeholder in a form ``_``\ *n* is simply a synonym for the corresponding 
``arg<n>`` specialization. The unnamed placeholder ``_`` (underscore) carries 
`special meaning`__ in bind and lambda expressions, and does not have 
defined semantics outside of these contexts.

Placeholder names can be made available in the user namespace through 
``using namespace mpl::placeholders;`` directive.

__ `bind semantics`_

Header
------

.. parsed-literal::
    
    #include <boost/mpl/placeholders.hpp>

|Note:| The include might be omitted when using placeholders to construct a |Lambda
Expression| for passing it to MPL's own algorithm or metafunction: any library 
component that is documented to accept a lambda expression makes the placeholders 
implicitly available for the user code |-- end note|


Parameters
----------

None.


Expression semantics
--------------------

For any integral constant ``n`` in the range [1, |BOOST_MPL_LIMIT_METAFUNCTION_ARITY|\] and
arbitrary types |a1...an|:


.. parsed-literal::

    typedef apply_wrap\ *n*\<_\ *n*\,a1,\ |...|\a\ *n*\ >::type x;

:Return type:
    A type.

:Semantics:
    Equivalent to
    
    .. parsed-literal::
    
        typedef apply_wrap\ *n*\< arg<\ *n*\ >,a1,\ |...|\a\ *n* >::type x;
    

Example
-------

.. parsed-literal::
    
    typedef apply_wrap\ ``5``\< _1,bool,char,short,int,long >::type t1;
    typedef apply_wrap\ ``5``\< _3,bool,char,short,int,long >::type t3;
    
    BOOST_MPL_ASSERT(( is_same< t1, bool > ));
    BOOST_MPL_ASSERT(( is_same< t3, short > ));


See also
--------

|Composition and Argument Binding|, |arg|, |lambda|, |bind|, |apply|, |apply_wrap|


.. |placeholder| replace:: `placeholder`_

.. |_1| replace:: `_1`_
.. |_2| replace:: `_2`_
.. |_3| replace:: `_3`_
.. |_4| replace:: `_4`_
.. |_5| replace:: `_5`_

.. _`_1`: `Placeholders`_
.. _`_2`: `Placeholders`_
.. _`_3`: `Placeholders`_
.. _`_4`: `Placeholders`_
.. _`_5`: `Placeholders`_

.. |_1,_2,..._n| replace:: |_1|, |_2|, |_3|,\ |...|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
