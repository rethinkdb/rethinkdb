.. Metafunctions/Miscellaneous//min |80

min
===

Synopsis
--------

.. parsed-literal::
    
    template<
          typename N1
        , typename N2
        >
    struct min
    {
        typedef |unspecified| type;
    };



Description
-----------

Returns the smaller of its two arguments.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/min_max.hpp>


Model of
--------

|Metafunction|


Parameters
----------

+---------------+-------------------+-------------------------------------------+
| Parameter     | Requirement       | Description                               |
+===============+===================+===========================================+
| ``N1``, ``N2``| Any type          | Types to compare.                         |
+---------------+-------------------+-------------------------------------------+


Expression semantics
--------------------

For arbitrary types ``x`` and ``y``:


.. parsed-literal::

    typedef min<x,y>::type r;


:Return type:
    A type.

:Precondition:
    ``less<x,y>::value`` is a well-formed integral constant expression.

:Semantics:
    Equivalent to
        
    .. parsed-literal::
    
        typedef if_< less<x,y>,x,y >::type r;



Complexity
----------

Constant time. 


Example
-------

.. parsed-literal::
    
    typedef fold<
          vector_c<int,1,7,0,-2,5,-1>
        , int_<-10>
        , min<_1,_2>
        >::type r;
    
    BOOST_MPL_ASSERT(( is_same< r, int_<-10> > ));


See also
--------

|Metafunctions|, |Comparison|, |max|, |less|, |min_element|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
