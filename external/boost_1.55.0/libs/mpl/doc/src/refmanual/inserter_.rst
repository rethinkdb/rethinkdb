.. Algorithms/Inserters//inserter

.. _`inserter_`:

inserter (class)
================

Synopsis
--------

.. parsed-literal::
    
    template<
          typename State
        , typename Operation
        >
    struct inserter
    {
        typedef State state;
        typedef Operation operation;
    };


Description
-----------

A general-purpose model of the |Inserter| concept. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/inserter.hpp>


Model of
--------

|Inserter|


Parameters
----------

+---------------+-------------------------------+-----------------------------------+
| Parameter     | Requirement                   | Description                       |
+===============+===============================+===================================+
| ``State``     | Any type                      | A initial state.                  |
+---------------+-------------------------------+-----------------------------------+
| ``Operation`` | Binary |Lambda Expression|    | An output operation.              |     
+---------------+-------------------------------+-----------------------------------+

Expression semantics
--------------------

|Semantics disclaimer...| |Inserter|.

For any binary |Lambda Expression| ``op`` and arbitrary type ``state``:

+---------------------------+-------------------------------------------+
| Expression                | Semantics                                 |
+===========================+===========================================+
| ``inserter<op,state>``    | An |Inserter|.                            |
+---------------------------+-------------------------------------------+

Complexity
----------

Amortized constant time.


Example
-------

.. parsed-literal::

    template< typename N > struct is_odd : bool_< ( N::value % 2 ) > {};
    
    typedef copy<
          range_c<int,0,10>
        , inserter< // a filtering 'push_back' inserter
              vector<>
            , if_< is_odd<_2>, push_back<_1,_2>, _1 >
            >
        >::type odds;
       
    BOOST_MPL_ASSERT(( equal< odds, vector_c<int,1,3,5,7,9>, equal_to<_,_> > ));


See also
--------

|Algorithms|, |Inserter|, |Reversible Algorithm|, |front_inserter|, |back_inserter|

.. |[inserter]| replace:: `inserter (class)`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
