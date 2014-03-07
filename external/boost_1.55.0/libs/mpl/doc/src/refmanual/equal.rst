.. Algorithms/Querying Algorithms//equal |100

equal
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Seq1
        , typename Seq2
        , typename Pred = is_same<_1,_2>
        >
    struct equal
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a true-valued |Integral Constant| if the two sequences ``Seq1`` 
and ``Seq2`` are identical when compared element-by-element.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/equal.hpp>



Parameters
----------

+-------------------+-------------------------------+-----------------------------------+
| Parameter         | Requirement                   | Description                       |
+===================+===============================+===================================+
| ``Seq1``, ``Seq2``| |Forward Sequence|            | Sequences to compare.             |
+-------------------+-------------------------------+-----------------------------------+
| ``Pred``          | Binary |Lambda Expression|    | A comparison criterion.           |
+-------------------+-------------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Sequence|\ s ``s1`` and ``s2`` and a binary |Lambda Expression| ``pred``:


.. parsed-literal::

    typedef equal<s1,s2,pred>::type c; 

:Return type:
    |Integral Constant|

:Semantics:
    ``c::value == true`` is and only if ``size<s1>::value == size<s2>::value`` 
    and for every iterator ``i`` in |begin/end<s1>| ``deref<i>::type`` is identical to 

    .. parsed-literal::

        advance< begin<s2>::type, distance< begin<s1>::type,i >::type >::type


Complexity
----------

Linear. At most ``size<s1>::value`` comparisons. 


Example
-------

.. parsed-literal::
    
    typedef vector<char,int,unsigned,long,unsigned long> s1;
    typedef list<char,int,unsigned,long,unsigned long> s2;
    
    BOOST_MPL_ASSERT(( equal<s1,s2> ));


See also
--------

|Querying Algorithms|, |find|, |find_if|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
