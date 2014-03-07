.. Algorithms/Transformation Algorithms//reverse_replace_if |150

reverse_replace_if
==================


Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Pred
        , typename In = |unspecified|
        >
    struct reverse_replace_if
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a reversed copy of the original sequence where every type that satisfies 
the predicate ``Pred`` has been replaced with ``NewType``. 

|transformation algorithm disclaimer|

Header
------

.. parsed-literal::
    
    #include <boost/mpl/replace_if.hpp>



Model of
--------

|Reversible Algorithm|


Parameters
----------

+---------------+-----------------------------------+-------------------------------+
| Parameter     | Requirement                       | Description                   |
+===============+===================================+===============================+
| ``Sequence``  | |Forward Sequence|                | An original sequence.         |
+---------------+-----------------------------------+-------------------------------+
| ``Pred``      | Unary |Lambda Expression|         | A replacement condition.      |
+---------------+-----------------------------------+-------------------------------+
| ``NewType``   | Any type                          | A type to replace with.       |
+---------------+-----------------------------------+-------------------------------+
| ``In``        | |Inserter|                        | An inserter.                  |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, an unary |Lambda Expression| ``pred``,
an |Inserter| ``in``, and arbitrary type ``x``:


.. parsed-literal::

    typedef reverse_replace_if<s,pred,x,in>::type r; 

:Return type:
    A type.

:Semantics:
    Equivalent to 

    .. parsed-literal::
        
        typedef lambda<pred>::type p;
        typedef reverse_transform< s, if_< apply_wrap1<p,_1>,x,_1>, in >::type r; 


Complexity
----------

Linear. Performs exactly ``size<s>::value`` applications of ``pred``, and at most 
``size<s>::value`` insertions.


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,1,4,5,2,7,5,3,5> numbers;
    typedef vector_c<int,1,4,0,2,0,0,3,0> expected;
    typedef reverse_replace_if< 
          numbers
        , greater< _, int_<4> >
        , int_<0>
        , front_inserter< vector<> >
        >::type result;
    
    BOOST_MPL_ASSERT(( equal< result,expected, equal_to<_,_> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |replace_if|, |reverse_replace|, |remove_if|, |transform|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
