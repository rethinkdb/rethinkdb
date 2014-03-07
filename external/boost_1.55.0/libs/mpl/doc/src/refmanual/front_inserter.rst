.. Algorithms/Inserters//front_inserter

front_inserter
==============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq
        >
    struct front_inserter
    {
        // |unspecified|
        // |...|
    };


Description
-----------

Inserts elements at the beginning of the sequence.

Header
------

.. parsed-literal::
    
    #include <boost/mpl/front_inserter.hpp>

Model of
--------

|Inserter|


Parameters
----------

+---------------+-------------------------------+---------------------------------------+
| Parameter     | Requirement                   | Description                           |
+===============+===============================+=======================================+
| ``Seq``       | |Front Extensible Sequence|   | A sequence to bind the inserter to.   |
+---------------+-------------------------------+---------------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Inserter|.

For any |Front Extensible Sequence| ``s``:

+---------------------------+-------------------------------------------------------+
| Expression                | Semantics                                             |
+===========================+=======================================================+
| ``front_inserter<s>``     | An |Inserter| ``in``, equivalent to                   |
|                           |                                                       |
|                           | .. parsed-literal::                                   |
|                           |                                                       |
|                           |   struct in : inserter<s,push_front<_1,_2> > {};      |
+---------------------------+-------------------------------------------------------+


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::

    typedef reverse_copy<
          range_c<int,0,5>
        , front_inserter< vector_c<int,5,6,7,8,9> >
        >::type range;
       
    BOOST_MPL_ASSERT(( equal< range, range_c<int,0,10> > ));


See also
--------

|Algorithms|, |Inserter|, |Reversible Algorithm|, |[inserter]|, |back_inserter|, |push_front|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
