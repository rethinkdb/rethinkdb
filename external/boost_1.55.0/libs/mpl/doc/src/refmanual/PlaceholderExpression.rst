.. Metafunctions/Concepts//Placeholder Expression |40

Placeholder Expression
======================

Description
-----------

A |Placeholder Expression| is a type that is either a |placeholder| or a class
template specialization with at least one argument that itself is a 
|Placeholder Expression|.


Expression requirements
-----------------------

If ``X`` is a class template, and ``a1``,... ``an`` are arbitrary types, then
``X<a1,...,an>`` is a |Placeholder Expression| if and only if all of the following
conditions hold:

* At least one of the template arguments ``a1``,... ``an`` is a |placeholder|
  or a |Placeholder Expression|.
  
* All of ``X``\ 's template parameters, including the default ones, are types.

* The number of ``X``\ 's template parameters, including the default ones, is
  less or equal to the value of ``BOOST_MPL_LIMIT_METAFUNCTION_ARITY`` 
  `configuration macro`__.

__ `Configuration`_


Models
------

* |_1|
* ``plus<_, int_<2> >``
* ``if_< less<_1, int_<7> >, plus<_1,_2>, _1 >``


See also
--------

|Lambda Expression|, |Placeholders|, |Metafunctions|, |apply|, |lambda|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
