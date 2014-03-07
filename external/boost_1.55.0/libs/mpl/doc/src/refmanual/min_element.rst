.. Algorithms/Querying Algorithms//min_element |80

min_element
===========

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename Pred = less<_1,_2>
        >
    struct min_element
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns an iterator to the smallest element in ``Sequence``. 


Header
------

.. parsed-literal::
    
    #include <boost/mpl/min_element.hpp>


Parameters
----------

+---------------+-------------------------------+-----------------------------------+
| Parameter     | Requirement                   | Description                       |
+===============+===============================+===================================+
|``Sequence``   | |Forward Sequence|            | A sequence to be searched.        |
+---------------+-------------------------------+-----------------------------------+
| ``Pred``      | Binary |Lambda Expression|    | A comparison criteria.            |
+---------------+-------------------------------+-----------------------------------+


Expression semantics
--------------------

For any |Forward Sequence| ``s`` and binary |Lambda Expression| ``pred``:

.. parsed-literal::

    typedef min_element<s,pred>::type i; 

:Return type:
    |Forward Iterator|.

:Semantics:
    ``i`` is the first iterator in |begin/end<s>| such that for every iterator ``j`` 
    in |begin/end<s>|,
    
    .. parsed-literal::
     
        apply< pred, deref<j>::type, deref<i>::type >::type::value == false



Complexity
----------

Linear. Zero comparisons if ``s`` is empty, otherwise exactly ``size<s>::value - 1``
comparisons. 


Example
-------

.. parsed-literal::
    
    typedef vector<bool,char[50],long,double> types;
    typedef min_element<
          transform_view< types,sizeof_<_1> >
        >::type iter;
    
    BOOST_MPL_ASSERT(( is_same< deref<iter::base>::type, bool> ));


See also
--------

|Querying Algorithms|, |max_element|, |find_if|, |upper_bound|, |find|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
