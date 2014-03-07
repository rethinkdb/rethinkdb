.. Algorithms/Transformation Algorithms//copy |10

copy
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename In = |unspecified|
        >
    struct copy
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a copy of the original sequence. 

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

+---------------+---------------------------+-------------------------------+
| Parameter     | Requirement               | Description                   |
+===============+===========================+===============================+
| ``Sequence``  | |Forward Sequence|        | A sequence to copy.           |
+---------------+---------------------------+-------------------------------+
| ``In``        | |Inserter|                | An inserter.                  |
+---------------+---------------------------+-------------------------------+


Expression semantics
--------------------

|Semantics disclaimer...| |Reversible Algorithm|.

For any |Forward Sequence| ``s``, and an |Inserter| ``in``:

.. parsed-literal::

    typedef copy<s,in>::type r; 

:Return type:
    A type.

:Semantics:
    Equivalent to 

    .. parsed-literal::
        
        typedef fold< s,in::state,in::operation >::type r;



Complexity
----------

Linear. Exactly ``size<s>::value`` applications of ``in::operation``. 


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,0,1,2,3,4,5,6,7,8,9> numbers;
    typedef copy<
          range_c<int,10,20>
        , back_inserter< numbers >
        >::type result;
    
    BOOST_MPL_ASSERT_RELATION( size<result>::value, ==, 20 );
    BOOST_MPL_ASSERT(( equal< result,range_c<int,0,20> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |reverse_copy|, |copy_if|, |transform|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
