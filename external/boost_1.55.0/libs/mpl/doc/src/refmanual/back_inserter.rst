.. Algorithms/Inserters//back_inserter

back_inserter
=============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq
        >
    struct back_inserter
    {
        // |unspecified|
        // |...|
    };


Description
-----------

Inserts elements at the end of the sequence.

Header
------

.. parsed-literal::
    
    #include <boost/mpl/back_inserter.hpp>

Model of
--------

|Inserter|


Parameters
----------

+---------------+-------------------------------+---------------------------------------+
| Parameter     | Requirement                   | Description                           |
+===============+===============================+=======================================+
| ``Seq``       | |Back Extensible Sequence|    | A sequence to bind the inserter to.   |
+---------------+-------------------------------+---------------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Inserter|.

For any |Back Extensible Sequence| ``s``:

+---------------------------+-------------------------------------------------------+
| Expression                | Semantics                                             |
+===========================+=======================================================+
| ``back_inserter<s>``      | An |Inserter| ``in``, equivalent to                   |
|                           |                                                       |
|                           | .. parsed-literal::                                   |
|                           |                                                       |
|                           |   struct in : inserter<s,push_back<_1,_2> > {};       |
+---------------------------+-------------------------------------------------------+


Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::

    typedef copy<
          range_c<int,5,10>
        , back_inserter< vector_c<int,0,1,2,3,4> >
        >::type range;
       
    BOOST_MPL_ASSERT(( equal< range, range_c<int,0,10> > ));


See also
--------

|Algorithms|, |Inserter|, |Reversible Algorithm|, |[inserter]|, |front_inserter|, |push_back|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
