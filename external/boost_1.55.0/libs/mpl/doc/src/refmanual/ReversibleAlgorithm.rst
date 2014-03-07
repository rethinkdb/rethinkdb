.. Algorithms/Concepts//Reversible Algorithm

Reversible Algorithm
====================

Description
-----------

A |Reversible Algorithm| is a member of a pair of
transformation algorithms that iterate over their input sequence(s) 
in opposite directions. For each reversible 
algorithm ``x`` there exists a *counterpart* algorithm ``reverse_x``, 
that exhibits the exact semantics of ``x`` except that the elements 
of its input sequence argument(s) are processed in the reverse 
order.


Expression requirements
-----------------------

.. |s1...sn| replace:: *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`

.. |s1...sn>::type| replace:: |s1...sn|, ...\ ``>::type``
.. |s1...sn,in>::type| replace:: |s1...sn|, ... ``in>::type``

|In the following table...| ``x`` is a placeholder token for the actual 
|Reversible Algorithm|'s name, |s1...sn| are 
|Forward Sequence|\ s, and ``in`` is an |Inserter|.

+---------------------------------------+-----------------------+-------------------+
| Expression                            | Type                  | Complexity        |
+=======================================+=======================+===================+
|``x<``\ |s1...sn>::type|               | |Forward Sequence|    | Unspecified.      |
+---------------------------------------+-----------------------+-------------------+
|``x<``\ |s1...sn,in>::type|            | Any type              | Unspecified.      |
+---------------------------------------+-----------------------+-------------------+
|``reverse_x<``\ |s1...sn>::type|       | |Forward Sequence|    | Unspecified.      |
+---------------------------------------+-----------------------+-------------------+
|``reverse_x<``\ |s1...sn,in>::type|    | Any type              | Unspecified.      |
+---------------------------------------+-----------------------+-------------------+


Expression semantics
--------------------

.. parsed-literal::

    typedef x<\ *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,...>::type t;

:Precondition:
    *s*\ :sub:`1` is an |Extensible Sequence|.

:Semantics:
    ``t`` is equivalent to
    
    .. parsed-literal::

        x<
              *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,...
            , back_inserter< clear<\ *s*\ :sub:`1`>::type >    
            >::type
            
    if ``has_push_back<``\ *s*\ :sub:`1`\ ``>::value == true`` and

    .. parsed-literal::

        reverse_x<
              *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,...
            , front_inserter< clear<\ *s*\ :sub:`1`>::type >    
            >::type

    otherwise.

.. ..........................................................................


.. parsed-literal::

    typedef x<\ *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,...\ in>::type t;

:Semantics:
    ``t`` is the result of an ``x`` invocation with arguments 
    *s*\ :sub:`1`,\ *s*\ :sub:`2`,... \ *s*\ :sub:`n`,...\ ``in``.


.. ..........................................................................


.. parsed-literal::

    typedef reverse_x<\ *s*\ :sub:`1`,\ *s*\ :sub:`2`,... \ *s*\ :sub:`n`,... >::type t;

:Precondition:
    *s*\ :sub:`1` is an |Extensible Sequence|.

:Semantics:
    ``t`` is equivalent to
    
    .. parsed-literal::

        x<
              *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,...
            , front_inserter< clear<\ *s*\ :sub:`1`>::type >    
            >::type
            
    if ``has_push_front<``\ *s*\ :sub:`1`\ ``>::value == true`` and

    .. parsed-literal::

        reverse_x<
              *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,...
            , back_inserter< clear<\ *s*\ :sub:`1`>::type >    
            >::type

    otherwise.


.. ..........................................................................

.. parsed-literal::

    typedef reverse_x<\ *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,... in>::type t;

:Semantics:
    ``t`` is the result of a ``reverse_x`` invocation with arguments 
    *s*\ :sub:`1`,\ *s*\ :sub:`2`,...\ *s*\ :sub:`n`,...\ ``in``.


Example
-------

.. parsed-literal::

    typedef transform< 
          range_c<int,0,10>
        , plus<_1,int_<7> >
        , back_inserter< vector0<> > 
        >::type r1;
    
    typedef transform< r1, minus<_1,int_<2> > >::type r2;
    typedef reverse_transform< 
          r2
        , minus<_1,5> 
        , front_inserter< vector0<> > 
        >::type r3;

    BOOST_MPL_ASSERT(( equal<r1, range_c<int,7,17> > ));
    BOOST_MPL_ASSERT(( equal<r2, range_c<int,5,15> > ));
    BOOST_MPL_ASSERT(( equal<r3, range_c<int,0,10> > ));


Models
------

* |transform|
* |remove|
* |replace|

See also
--------

|Transformation Algorithms|, |Inserter|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
