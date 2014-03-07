.. Sequences/Intrinsic Metafunctions//insert

insert
======

Synopsis
--------

.. parsed-literal::

    template< 
          typename Sequence
        , typename Pos
        , typename T 
        >
    struct insert
    {
        typedef |unspecified| type;
    };


    template< 
          typename Sequence
        , typename T 
        >
    struct insert
    {
        typedef |unspecified| type;
    };


Description
-----------

``insert`` is an |overloaded name|:

* ``insert<Sequence,Pos,T>`` performs an insertion of
  type ``T`` at an arbitrary position ``Pos`` in ``Sequence``. ``Pos`` is ignored is
  ``Sequence`` is a model of |Extensible Associative Sequence|. 

* ``insert<Sequence,T>`` is a shortcut notation for ``insert<Sequence,Pos,T>`` for the 
  case when ``Sequence`` is a model of |Extensible Associative Sequence|.


Header
------

.. parsed-literal::
    
    #include <boost/mpl/insert.hpp>


Model of
--------

|Tag Dispatched Metafunction|


Parameters
----------

+---------------+-----------------------------------+-----------------------------------------------+
| Parameter     | Requirement                       | Description                                   |
+===============+===================================+===============================================+
| ``Sequence``  | |Extensible Sequence| or          | A sequence to insert into.                    |
|               | |Extensible Associative Sequence| |                                               |
+---------------+-----------------------------------+-----------------------------------------------+
| ``Pos``       | |Forward Iterator|                | An iterator in ``Sequence`` specifying the    |
|               |                                   | insertion position.                           |
+---------------+-----------------------------------+-----------------------------------------------+
| ``T``         | Any type                          | The element to be inserted.                   |
+---------------+-----------------------------------+-----------------------------------------------+


Expression semantics
--------------------

.. compound::
    :class: expression-semantics

    For any |Extensible Sequence| ``s``, iterator ``pos`` in ``s``, and arbitrary type ``x``:

    .. parsed-literal::

        typedef insert<s,pos,x>::type r; 

    :Return type:
        |Extensible Sequence|

    :Precondition:
        ``pos`` is an iterator in ``s``. 

    :Semantics:
        ``r`` is a sequence, |concept-identical| to ``s``, of the following elements: 
        [``begin<s>::type``, ``pos``), ``x``, [``pos``, ``end<s>::type``). 
        
    :Postcondition:
        The relative order of the elements in ``r`` is the same as in ``s``. 

        .. parsed-literal::

           at< r, distance< begin<s>::type,pos >::type >::type
        
        is identical to ``x``; 

        .. parsed-literal::

           size<r>::value == size<s>::value + 1;        
        


.. compound::
    :class: expression-semantics


    For any |Extensible Associative Sequence| ``s``, iterator ``pos`` in ``s``,
    and arbitrary type ``x``:


    .. parsed-literal::

        typedef insert<s,x>::type r; 

    :Return type:
        |Extensible Associative Sequence|

    :Semantics:
        ``r`` is |concept-identical| and equivalent to ``s``, except that
        ``at< r, key_type<s,x>::type >::type`` is identical to ``value_type<s,x>::type``.
        
    :Postcondition:
        ``size<r>::value == size<s>::value + 1``.


    .. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    .. parsed-literal::

        typedef insert<s,pos,x>::type r; 

    :Return type:
        |Extensible Associative Sequence|

    :Precondition:
        ``pos`` is an iterator in ``s``. 

    :Semantics:
        Equivalent to ``typedef insert<s,x>::type r``; ``pos`` is ignored.
        


Complexity
----------

+---------------------------------------+-----------------------------------------------+
| Sequence archetype                    | Complexity                                    |
+=======================================+===============================================+
| |Extensible Associative Sequence|     | Amortized constant time.                      |
+---------------------------------------+-----------------------------------------------+
| |Extensible Sequence|                 | Linear in the worst case, or amortized        |
|                                       | constant time.                                |
+---------------------------------------+-----------------------------------------------+


Example
-------

.. parsed-literal::
    
    typedef vector_c<int,0,1,3,4,5,6,7,8,9> numbers;
    typedef find< numbers,integral_c<int,3> >::type pos;
    typedef insert< numbers,pos,integral_c<int,2> >::type range;
    
    BOOST_MPL_ASSERT_RELATION( size<range>::value, ==, 10 );
    BOOST_MPL_ASSERT(( equal< range,range_c<int,0,10> > ));


.. parsed-literal::

    typedef map< mpl::pair<int,unsigned> > m;
    typedef insert<m,mpl::pair<char,long> >::type m1;
    
    BOOST_MPL_ASSERT_RELATION( size<m1>::value, ==, 2 );
    BOOST_MPL_ASSERT(( is_same< at<m1,int>::type,unsigned > ));
    BOOST_MPL_ASSERT(( is_same< at<m1,char>::type,long > ));


See also
--------

|Extensible Sequence|, |Extensible Associative Sequence|, |insert_range|, |push_front|, |push_back|, |erase|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
