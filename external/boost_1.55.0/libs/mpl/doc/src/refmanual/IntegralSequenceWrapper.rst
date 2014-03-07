.. Sequences/Concepts//Integral Sequence Wrapper |90

Integral Sequence Wrapper
=========================

Description
-----------

An |Integral Sequence Wrapper| is a class template that provides a concise
interface for creating a corresponding sequence of |Integral Constant|\ s. In
particular, assuming that ``seq`` is a name of the wrapper's underlying 
sequence and |c1...cn| are integral constants of an integral type ``T`` to 
be stored in the sequence, the wrapper provides us with the following 
notation:

    .. line-block::

        ``seq_c<T``,\ |c1...cn|\ ``>``

If ``seq`` is a |Variadic Sequence|, *numbered* wrapper forms are
also avaialable:

    .. line-block::

        ``seq``\ *n*\ ``_c<T``,\ |c1...cn|\ ``>``



Expression requirements
-----------------------

|In the following table...| ``seq`` is a placeholder token for the 
|Integral Sequence Wrapper|'s underlying sequence's name.


.. |seq_c| replace:: ``seq_c<T``,\ |c1...cn|
.. |seqn_c| replace:: ``seq``\ *n*\ ``_c<T``,\ |c1...cn|


+-------------------------------+-----------------------+---------------------------+
| Expression                    | Type                  | Complexity                |
+===============================+=======================+===========================+
| |seq_c|\ ``>``                | |Forward Sequence|    | Amortized constant time.  |
+-------------------------------+-----------------------+---------------------------+
| |seq_c|\ ``>::type``          | |Forward Sequence|    | Amortized constant time.  |
+-------------------------------+-----------------------+---------------------------+
| |seq_c|\ ``>::value_type``    | An integral type      | Amortized constant time.  |
+-------------------------------+-----------------------+---------------------------+
| |seqn_c|\ ``>``               | |Forward Sequence|    | Amortized constant time.  |
+-------------------------------+-----------------------+---------------------------+
| |seqn_c|\ ``>::type``         | |Forward Sequence|    | Amortized constant time.  |
+-------------------------------+-----------------------+---------------------------+
| |seqn_c|\ ``>::value_type``   | An integral type      | Amortized constant time.  |
+-------------------------------+-----------------------+---------------------------+


Expression semantics
--------------------


.. parsed-literal::

    typedef seq_c<T,\ |c1...cn|> s;
    typedef seq\ *n*\ _c<T,\ |c1...cn|> s;

:Semantics:
    ``s`` is a sequence ``seq`` of integral constant wrappers ``integral_c<T,``\ |c1|\ ``>``,
    ``integral_c<T,``\ |c2|\ ``>``, ... ``integral_c<T,``\ |cn|\ ``>``.

:Postcondition:
    ``size<s>::value == n``.

    .. .. parsed-literal::
    
        BOOST_MPL_ASSERT_RELATION(( at_c<v,0>::type::value,==,\ |c1| ));
        BOOST_MPL_ASSERT_RELATION(( at_c<v,1>::type::value,==,\ |c2| ));
        ...
        BOOST_MPL_ASSERT_RELATION(( at_c<v,\ *n*>::type::value,==,\ |cn| ));


.. ..........................................................................

.. parsed-literal::

    typedef seq_c<T,\ |c1...cn|>::type s;
    typedef seq\ *n*\ _c<T,\ |c1...cn|>::type s;

:Semantics:
    ``s`` is identical to 
    ``seq``\ *n*\ ``<``\ ``integral_c<T,``\ |c1|\ ``>``,\ ``integral_c<T,``\ |c2|\ ``>``, 
    ... ``integral_c<T,``\ |cn|\ ``>`` ``>``.


.. ..........................................................................

.. parsed-literal::

    typedef seq_c<T,\ |c1...cn|>::value_type t;
    typedef seq\ *n*\ _c<T,\ |c1...cn|>::value_type t;

:Semantics:
    ``is_same<t,T>::value == true``.


Models
------

* |vector_c|
* |list_c|
* |set_c|

See also
--------

|Sequences|, |Variadic Sequence|, |Integral Constant|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
