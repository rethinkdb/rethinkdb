.. Algorithms/Transformation Algorithms//reverse |100

reverse
=======

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename In = |unspecified|
        >
    struct reverse
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a reversed copy of the original sequence. ``reverse`` is a synonym for
|reverse_copy|.

|transformation algorithm disclaimer|

Header
------

.. parsed-literal::
    
    #include <boost/mpl/reverse.hpp>


Parameters
----------

+---------------+-----------------------------------+-------------------------------+
| Parameter     | Requirement                       | Description                   |
+===============+===================================+===============================+
| ``Sequence``  | |Forward Sequence|                | A sequence to reverse.        |
+---------------+-----------------------------------+-------------------------------+
| ``In``        | |Inserter|                        | An inserter.                  |
+---------------+-----------------------------------+-------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s``, and an |Inserter| ``in``:

.. parsed-literal::

    typedef reverse<s,in>::type r; 

:Return type:
    A type.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
        typedef reverse_copy<s,in>::type r; 


Complexity
----------

Linear.


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,9,8,7,6,5,4,3,2,1,0> numbers;
    typedef reverse< numbers >::type result;
    
    BOOST_MPL_ASSERT(( equal< result, range_c<int,0,10> > ));


See also
--------

|Transformation Algorithms|, |Reversible Algorithm|, |reverse_copy|, |copy|, |copy_if|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
