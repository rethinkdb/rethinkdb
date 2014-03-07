.. Sequences/Intrinsic Metafunctions//clear

clear
=====

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        >
    struct clear
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns an empty sequence |concept-identical| to ``Sequence``.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/clear.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------------------+---------------------------------------+
| Parameter     | Requirement                       | Description                           |
+===============+===================================+=======================================+
| ``Sequence``  | |Extensible Sequence| or          | A sequence to get an empty "copy" of. |
|               | |Extensible Associative Sequence| |                                       |
+---------------+-----------------------------------+---------------------------------------+


Expression semantics
--------------------

For any |Extensible Sequence| or |Extensible Associative Sequence| ``s``:


.. parsed-literal::

    typedef clear<s>::type t; 

:Return type:
    |Extensible Sequence| or |Extensible Associative Sequence|.

:Semantics:
    Equivalent to 

    .. parsed-literal::
    
       typedef erase< s, begin<s>::type, end<s>::type >::type t;


:Postcondition:
    ``empty<s>::value == true``.


Complexity
----------

Amortized constant time. 


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,1,3,5,7,9,11> odds;
    typedef clear<odds>::type nothing;
    
    BOOST_MPL_ASSERT(( empty<nothing> ));


See also
--------

|Extensible Sequence|, |Extensible Associative Sequence|, |erase|, |empty|, |begin|, |end|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
