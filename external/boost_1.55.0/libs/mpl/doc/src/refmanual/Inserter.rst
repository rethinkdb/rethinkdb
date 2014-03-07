.. Algorithms/Concepts//Inserter

Inserter
========

Description
-----------

An |Inserter| is a compile-time substitute for STL |Output Iterator|. 
Under the hood, it's simply a type holding 
two entities: a *state* and an *operation*. When passed to a 
|transformation algorithm|, the inserter's binary operation is
invoked for every element that would normally be written into the 
output iterator, with the element itself (as the second
argument) and the result of the previous operation's invocation |--| or,
for the very first element, the inserter's initial state. 

Technically, instead of taking a single inserter parameter,
|transformation algorithms| could accept the state and the "output"
operation separately. Grouping these in a single parameter entity, 
however, brings the algorithms semantically and syntactically closer to 
their STL counterparts, significantly simplifying many of the common 
use cases.


Valid expressions
-----------------

|In the following table...| ``in`` is a model of |Inserter|.

+-----------------------+-------------------------------+
| Expression            | Type                          |
+=======================+===============================+
| ``in::state``         | Any type                      |
+-----------------------+-------------------------------+
| ``in::operation``     | Binary |Lambda Expression|    |
+-----------------------+-------------------------------+


Expression semantics
--------------------

+-----------------------+-------------------------------------------+
| Expression            | Semantics                                 |
+=======================+===========================================+
| ``in::state``         | The inserter's initial state.             |
+-----------------------+-------------------------------------------+
| ``in::operation``     | The inserter's "output" operation.        |
+-----------------------+-------------------------------------------+


Example
-------

.. parsed-literal::

    typedef transform<
          range_c<int,0,10>
        , plus<_1,_1>
        , back_inserter< vector0<> >
        >::type result;


Models
------

* |[inserter]|
* |front_inserter|
* |back_inserter|

See also
--------

|Algorithms|, |Transformation Algorithms|, |[inserter]|, |front_inserter|, |back_inserter|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
