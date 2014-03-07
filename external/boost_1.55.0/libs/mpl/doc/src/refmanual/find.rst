.. Algorithms/Querying Algorithms//find |10

find
====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename T
        >
    struct find
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns an iterator to the first occurrence of type ``T`` in a ``Sequence``. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/find.hpp>



Parameters
----------

+---------------+-----------------------+-----------------------------------+
| Parameter     | Requirement           | Description                       |
+===============+=======================+===================================+
| ``Sequence``  | |Forward Sequence|    | A sequence to search in.          |
+---------------+-----------------------+-----------------------------------+
| ``T``         | Any type              | A type to search for.             |
+---------------+-----------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s`` and arbitrary type ``t``:


.. parsed-literal::

    typedef find<s,t>::type i;
    

:Return type:
    |Forward Iterator|.
    
:Semantics:
    Equivalent to

    .. parsed-literal::
    
        typedef find_if<s, is_same<_,t> >::type i;


Complexity
----------

Linear. At most ``size<s>::value`` comparisons for identity. 


Example
-------

.. parsed-literal::
    
    typedef vector<char,int,unsigned,long,unsigned long> types;
    typedef find<types,unsigned>::type iter;
    
    BOOST_MPL_ASSERT(( is_same< deref<iter>::type, unsigned > ));
    BOOST_MPL_ASSERT_RELATION( iter::pos::value, ==, 2 );


See also
--------

|Querying Algorithms|, |contains|, |find_if|, |count|, |lower_bound|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
