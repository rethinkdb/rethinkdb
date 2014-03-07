.. Algorithms/Transformation Algorithms//reverse_replace |140

reverse_replace
===============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename OldType
        , typename NewType
        , typename In = |unspecified|
        >
    struct reverse_replace
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a reversed copy of the original sequence where every type identical to ``OldType`` 
has been replaced with ``NewType``. 

|transformation algorithm disclaimer|

Header
------

.. parsed-literal::
    
    #include <boost/mpl/replace.hpp>


Model of
--------

|Reversible Algorithm|


Parameters
----------

+---------------+-----------------------------------+-------------------------------+
| Parameter     | Requirement                       | Description                   |
+===============+===================================+===============================+
| ``Sequence``  | |Forward Sequence|                | A original sequence.          |
+---------------+-----------------------------------+-------------------------------+
| ``OldType``   | Any type                          | A type to be replaced.        |
+---------------+-----------------------------------+-------------------------------+
| ``NewType``   | Any type                          | A type to replace with.       |
+---------------+-----------------------------------+-------------------------------+
| ``In``        | |Inserter|                        | An inserter.                  |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, an |Inserter| ``in``, and arbitrary types ``x`` and ``y``:


.. parsed-literal::

    typedef reverse_replace<s,x,y,in>::type r; 

:Return type:
    A type.
 
:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef reverse_replace_if< s,y,is_same<_,x>,in >::type r; 


Complexity
----------

Linear. Performs exactly ``size<s>::value`` comparisons for 
identity / insertions.


Example
-------

.. parsed-literal::
    
    typedef vector<int,float,char,float,float,double> types;
    typedef vector<double,double,double,char,double,int> expected;
    typedef reverse_replace< types,float,double >::type result;
    
    BOOST_MPL_ASSERT(( equal< result,expected > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |replace|, |reverse_replace_if|, |remove|, |reverse_transform|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
