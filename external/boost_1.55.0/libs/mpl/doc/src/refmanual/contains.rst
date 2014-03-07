.. Algorithms/Querying Algorithms//contains |30

contains
========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename T
        >
    struct contains
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns a true-valued |Integral Constant| if one or more elements in ``Sequence`` 
are identical to ``T``. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/contains.hpp>


Parameters
----------

+---------------+---------------------------+-----------------------------------+
| Parameter     | Requirement               | Description                       |
+===============+===========================+===================================+
| ``Sequence``  | |Forward Sequence|        | A sequence to be examined.        |
+---------------+---------------------------+-----------------------------------+
| ``T``         | Any type                  | A type to search for.             |
+---------------+---------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s`` and arbitrary type ``t``:


.. parsed-literal::

    typedef contains<s,t>::type r;


:Return type:
    |Integral Constant|.

:Semantics:
    Equivalent to

    .. parsed-literal::
        
        typedef not_< is_same< 
              find<s,t>::type
            , end<s>::type
            > >::type r;


Complexity
----------

Linear. At most ``size<s>::value`` comparisons for identity. 


Example
-------

.. parsed-literal::
    
    typedef vector<char,int,unsigned,long,unsigned long> types;
    BOOST_MPL_ASSERT_NOT(( contains<types,bool> ));


See also
--------

|Querying Algorithms|, |find|, |find_if|, |count|, |lower_bound|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
