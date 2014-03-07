.. Sequences/Classes//deque |30

deque
=====

Description
-----------

``deque`` is a |variadic|, `random access`__, `extensible`__ sequence of types that 
supports constant-time insertion and removal of elements at both ends, and 
linear-time insertion and removal of elements in the middle. In this implementation 
of the library, ``deque`` is a synonym for |vector|.

__ `Random Access Sequence`_
__ `Extensible Sequence`_

Header
------

.. parsed-literal::

    #include <boost/mpl/deque.hpp>


Model of
--------

* |Variadic Sequence|
* |Random Access Sequence|
* |Extensible Sequence|
* |Back Extensible Sequence|
* |Front Extensible Sequence|


Expression semantics
--------------------

See |vector| specification.


Example
-------

.. parsed-literal::
    
    typedef deque<float,double,long double> floats;
    typedef push_back<floats,int>::type types;

    BOOST_MPL_ASSERT(( |is_same|\< at_c<types,3>::type, int > ));


See also
--------

|Sequences|, |vector|, |list|, |set|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
