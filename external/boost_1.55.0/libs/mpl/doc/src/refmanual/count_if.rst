.. Algorithms/Querying Algorithms//count_if |50

count_if
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Pred
        >
    struct count_if
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the number of elements in ``Sequence`` that satisfy the predicate ``Pred``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/count_if.hpp>



Parameters
----------

+---------------+-------------------------------+-----------------------------------+
| Parameter     | Requirement                   | Description                       |
+===============+===============================+===================================+
| ``Sequence``  | |Forward Sequence|            | A sequence to be examined.        |
+---------------+-------------------------------+-----------------------------------+
| ``Pred``      | Unary |Lambda Expression|     | A count condition.                |
+---------------+-------------------------------+-----------------------------------+


Expression semantics
--------------------


For any |Forward Sequence| ``s`` and unary |Lambda Expression| ``pred``:

.. parsed-literal::

    typedef count_if<s,pred>::type n; 

:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to 
        
    .. parsed-literal::
    
        typedef lambda<pred>::type p;
        typedef fold< 
              s
            , long_<0>
            , if_< apply_wrap\ ``1``\<p,_2>, next<_1>, _1 >
            >::type n;


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``pred``. 


Example
-------

.. parsed-literal::
    
    typedef vector<int,char,long,short,char,long,double,long> types;
        
    BOOST_MPL_ASSERT_RELATION( (count_if< types, is_float<_> >::value), ==, 1 );
    BOOST_MPL_ASSERT_RELATION( (count_if< types, is_same<_,char> >::value), ==, 2 );
    BOOST_MPL_ASSERT_RELATION( (count_if< types, is_same<_,void> >::value), ==, 0 );


See also
--------

|Querying Algorithms|, |count|, |find|, |find_if|, |contains|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
