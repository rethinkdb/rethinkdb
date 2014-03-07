.. Algorithms/Transformation Algorithms//reverse_remove |160

reverse_remove
==============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename T
        , typename In = |unspecified|
        >
    struct reverse_remove
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a new sequence that contains all elements from |begin/end<Sequence>|
range in reverse order except those that are identical to ``T``.

|transformation algorithm disclaimer|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/remove.hpp>


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
| ``T``         | Any type                          | A type to be removed.         |
+---------------+-----------------------------------+-------------------------------+
| ``In``        | |Inserter|                        | An inserter.                  |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, an |Inserter| ``in``, and arbitrary type ``x``:


.. parsed-literal::

    typedef reverse_remove<s,x,in>::type r; 

:Return type:
    A type.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef reverse_remove_if< s,is_same<_,x>,in >::type r;


Complexity
----------

Linear. Performs exactly ``size<s>::value`` comparisons for equality, and at 
most ``size<s>::value`` insertions.


Example
-------

.. parsed-literal::
    
    typedef vector<int,float,char,float,float,double>::type types;
    typedef reverse_remove< types,float >::type result;

    BOOST_MPL_ASSERT(( equal< result, vector<double,char,int> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |remove|, |reverse_remove_if|, 
|reverse_copy|, |transform|, |replace|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
