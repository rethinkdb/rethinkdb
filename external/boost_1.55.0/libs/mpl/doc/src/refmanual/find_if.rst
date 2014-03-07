.. Algorithms/Querying Algorithms//find_if |20

find_if
=======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Pred
        >
    struct find_if
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns an iterator to the first element in ``Sequence`` that satisfies 
the predicate ``Pred``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/find_if.hpp>



Parameters
----------

+---------------+-------------------------------+-----------------------------------+
| Parameter     | Requirement                   | Description                       |
+===============+===============================+===================================+
| ``Sequence``  | |Forward Sequence|            | A sequence to search in.          |
+---------------+-------------------------------+-----------------------------------+
| ``Pred``      | Unary |Lambda Expression|     | A search condition.               |
+---------------+-------------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s`` and unary |Lambda Expression| ``pred``:


.. parsed-literal::

    typedef find_if<s,pred>::type i; 

:Return type:
    |Forward Iterator|.

:Semantics:
    ``i`` is the first iterator in the range |begin/end<s>| such that 

    .. parsed-literal::
        
        apply< pred,deref<i>::type >::type::value == true
        
    If no such iterator exists, ``i`` is identical to ``end<s>::type``.


Complexity
----------

Linear. At most ``size<s>::value`` applications of ``pred``. 


Example
-------

.. parsed-literal::
    
    typedef vector<char,int,unsigned,long,unsigned long> types;
    typedef find_if<types, is_same<_1,unsigned> >::type iter;

    BOOST_MPL_ASSERT(( is_same< deref<iter>::type, unsigned > ));
    BOOST_MPL_ASSERT_RELATION( iter::pos::value, ==, 2 );


See also
--------

|Querying Algorithms|, |find|, |count_if|, |lower_bound|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
