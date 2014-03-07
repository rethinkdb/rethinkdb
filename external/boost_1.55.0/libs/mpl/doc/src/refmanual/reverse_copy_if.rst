.. Algorithms/Transformation Algorithms//reverse_copy_if |120

reverse_copy_if
===============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Pred
        , typename In = |unspecified|
        >
    struct reverse_copy_if
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a reversed, filtered copy of the original sequence containing the 
elements that satisfy the predicate ``Pred``.

|transformation algorithm disclaimer|

Header
------

.. parsed-literal::
    
    #include <boost/mpl/copy_if.hpp>


Model of
--------

|Reversible Algorithm|


Parameters
----------

+---------------+-----------------------------------+-------------------------------+
| Parameter     | Requirement                       | Description                   |
+===============+===================================+===============================+
| ``Sequence``  | |Forward Sequence|                | A sequence to copy.           |
+---------------+-----------------------------------+-------------------------------+
| ``Pred``      | Unary |Lambda Expression|         | A copying condition.          |
+---------------+-----------------------------------+-------------------------------+
| ``In``        | |Inserter|                        | An inserter.                  |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, an unary |Lambda Expression| ``pred``, and 
an |Inserter| ``in``:


.. parsed-literal::

    typedef reverse_copy_if<s,pred,in>::type r; 


:Return type:
    A type 

:Semantics:
    Equivalent to 
        
    .. parsed-literal::
        
        typedef lambda<pred>::type p;
        typedef lambda<in::operation>::type op;
        
        typedef reverse_fold<
              s
            , in::state
            , eval_if<
                  apply_wrap\ ``1``\<p,_2>
                , apply_wrap\ ``2``\<op,_1,_2>
                , identity<_1>
                >
            >::type r;


Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``pred``, and at 
most ``size<s>::value`` applications of ``in::operation``. 


Example
-------

.. parsed-literal::
    
    typedef reverse_copy_if<
          range_c<int,0,10>
        , less< _1, int_<5> >
        , front_inserter< vector<> >
        >::type result;
    
    BOOST_MPL_ASSERT_RELATION( size<result>::value, ==, 5 );
    BOOST_MPL_ASSERT(( equal<result,range_c<int,0,5> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |copy_if|, |reverse_copy|, |remove_if|, |replace_if|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
