.. Sequences/Intrinsic Metafunctions//at

at
==

Synopsis
--------

.. parsed-literal::
    
    template<
          typename Sequence
        , typename N
        >
    struct at
    {
        typedef |unspecified| type;
    };

    template<
          typename AssocSeq
        , typename Key
        , typename Default = |unspecified|
        >
    struct at
    {
        typedef |unspecified| type;
    };


Description
-----------

``at`` is an |overloaded name|:

* ``at<Sequence,N>`` returns the ``N``-th element from the beginning of the 
  |Forward Sequence| ``Sequence``.

* ``at<AssocSeq,Key,Default>`` returns the first element associated with ``Key`` 
  in the |Associative Sequence| ``AssocSeq``, or ``Default`` if no such element
  exists.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/at.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+---------------------------+-----------------------------------------------+
| Parameter     | Requirement               | Description                                   |
+===============+===========================+===============================================+
| ``Sequence``  | |Forward Sequence|        | A sequence to be examined.                    |
+---------------+---------------------------+-----------------------------------------------+
| ``AssocSeq``  | |Associative Sequence|    | A sequence to be examined.                    |
+---------------+---------------------------+-----------------------------------------------+
| ``N``         | |Integral Constant|       | An offset from the beginning of the sequence  |
|               |                           | specifying the element to be retrieved.       |
+---------------+---------------------------+-----------------------------------------------+
| ``Key``       | Any type                  | A key for the element to be retrieved.        |
+---------------+---------------------------+-----------------------------------------------+
| ``Default``   | Any type                  | A default value to return if the element is   |
|               |                           | not found.                                    |
+---------------+---------------------------+-----------------------------------------------+


Expression semantics
--------------------

.. compound::
    :class: expression-semantics

    For any |Forward Sequence| ``s``, and |Integral Constant| ``n``:

    .. parsed-literal::

        typedef at<s,n>::type t; 

    :Return type:
        A type.

    :Precondition:
        ``0 <= n::value < size<s>::value``.

    :Semantics:
        Equivalent to 
        
        .. parsed-literal::
           
           typedef deref< advance< begin<s>::type,n >::type >::type t;


.. compound::
    :class: expression-semantics

    For any |Associative Sequence| ``s``, and arbitrary types ``key`` and ``x``:

    .. parsed-literal::

        typedef at<s,key,x>::type t; 

    :Return type:
        A type.

    :Semantics:
        If ``has_key<s,key>::value == true``, ``t`` is the value type associated with ``key``;
        otherwise ``t`` is identical to ``x``.


    .. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    .. parsed-literal::

        typedef at<s,key>::type t; 
    
    :Return type:
        A type.

    :Semantics:
        Equivalent to

        .. parsed-literal::
           
           typedef at<s,key,void\_>::type t;


Complexity
----------

+-------------------------------+-----------------------------------+
| Sequence archetype            | Complexity                        |
+===============================+===================================+
| |Forward Sequence|            | Linear.                           |
+-------------------------------+-----------------------------------+
| |Random Access Sequence|      | Amortized constant time.          |
+-------------------------------+-----------------------------------+
| |Associative Sequence|        | Amortized constant time.          |
+-------------------------------+-----------------------------------+

Example
-------

.. parsed-literal::
    
    typedef range_c<long,10,50> range;
    BOOST_MPL_ASSERT_RELATION( (at< range, int_<0> >::value), ==, 10 );
    BOOST_MPL_ASSERT_RELATION( (at< range, int_<10> >::value), ==, 20 );
    BOOST_MPL_ASSERT_RELATION( (at< range, int_<40> >::value), ==, 50 );


.. parsed-literal::

    typedef set< int const,long*,double > s;

    BOOST_MPL_ASSERT(( is_same< at<s,char>::type, void\_ > ));
    BOOST_MPL_ASSERT(( is_same< at<s,int>::type, int > ));


See also
--------

|Forward Sequence|, |Random Access Sequence|, |Associative Sequence|, |at_c|, |front|, |back|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
