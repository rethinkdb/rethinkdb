.. Algorithms/Transformation Algorithms//reverse_copy |110

reverse_copy
============

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename In = |unspecified|
        >
    struct reverse_copy
    {
        typedef |unspecified| type;
    };


Description
-----------

Returns a reversed copy of the original sequence.

|transformation algorithm disclaimer|


Header
------

.. parsed-literal::
    
    #include <boost/mpl/copy.hpp>


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
| ``In``        | |Inserter|                        | An inserter.                  |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, and an |Inserter| ``in``:

.. parsed-literal::

    typedef reverse_copy<s,in>::type r; 

:Return type:
    A type.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef reverse_fold< s,in::state,in::operation >::type r; 



Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``in::operation``. 


Example
-------

.. parsed-literal::

    typedef list_c<int,10,11,12,13,14,15,16,17,18,19>::type numbers;
    typedef reverse_copy<
          range_c<int,0,10>
        , front_inserter< numbers >
        >::type result;
    
    BOOST_MPL_ASSERT_RELATION( size<result>::value, ==, 20 );
    BOOST_MPL_ASSERT(( equal< result,range_c<int,0,20> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |copy|, |reverse_copy_if|, |reverse_transform|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
