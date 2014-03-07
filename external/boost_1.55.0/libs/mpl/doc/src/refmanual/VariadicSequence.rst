.. Sequences/Concepts//Variadic Sequence |100

Variadic Sequence
=================

Description
-----------

A |Variadic Sequence| is a member of a family of sequence classes with both 
*variadic* and *numbered* forms. If ``seq`` is a generic name for some 
|Variadic Sequence|, its *variadic form* allows us to specify a sequence of 
*n* elements |t1...tn|, for any *n* from 0 up to a 
`preprocessor-configurable limit`__ ``BOOST_MPL_LIMIT_``\ *seq*\ ``_SIZE``, 
using the following notation:

__ `Configuration`_

    .. line-block::

        ``seq<``\ |t1...tn|\ ``>``

By contrast, each *numbered* sequence form accepts the exact number of elements 
that is encoded in the name of the corresponding class template:

    .. line-block::

        ``seq``\ *n*\ ``<``\ |t1...tn|\ ``>``

For numbered forms, there is no predefined top limit for *n*, aside from compiler 
limitations on the number of template parameters.

.. The variadic form of sequence ``seq`` is defined in 
   ``<boost/mpl/``\ *seq*\ ``.hpp>`` header.
   The numbered forms are defined in batches of 10. 


Expression requirements
-----------------------

|In the following table...| ``seq`` is a placeholder token for the actual 
|Variadic Sequence| name.

.. |seq<t1...tn>| replace:: ``seq<``\ |t1...tn|\ ``>``
.. |seq<t1...tn>::type| replace:: ``seq<``\ |t1...tn|\ ``>::type``

.. |seqn<t1...tn>| replace:: ``seq``\ *n*\ ``<``\ |t1...tn|\ ``>``
.. |seqn<t1...tn>::type| replace:: ``seq``\ *n*\ ``<``\ |t1...tn|\ ``>::type``


+---------------------------+-----------------------+---------------------------+
| Expression                | Type                  | Complexity                |
+===========================+=======================+===========================+
| |seq<t1...tn>|            | |Forward Sequence|    | Amortized constant time   |
+---------------------------+-----------------------+---------------------------+
| |seq<t1...tn>::type|      | |Forward Sequence|    | Amortized constant time   |
+---------------------------+-----------------------+---------------------------+
| |seqn<t1...tn>|           | |Forward Sequence|    | Amortized constant time   |
+---------------------------+-----------------------+---------------------------+
| |seqn<t1...tn>::type|     | |Forward Sequence|    | Amortized constant time   |
+---------------------------+-----------------------+---------------------------+


Expression semantics
--------------------


.. parsed-literal::

    typedef seq<|t1...tn|> s;
    typedef seq\ *n*\ <|t1...tn|> s;

:Semantics:
    ``s`` is a sequence of elements |t1...tn|.

:Postcondition:
    ``size<s>::value == n``.

    .. FIXME .. parsed-literal::
    
        BOOST_MPL_ASSERT((|is_same|\< at_c<v,0>::type,\ |t1| >));
        BOOST_MPL_ASSERT((|is_same|\< at_c<v,1>::type,\ |t2| >));
        ...
        BOOST_MPL_ASSERT((|is_same|\< at_c<v,\ *n*>::type,\ |tn| >));

.. ..........................................................................

.. parsed-literal::

    typedef seq<|t1...tn|>::type s;
    typedef seq\ *n*\ <|t1...tn|>::type s;

:Semantics:
    ``s`` is identical to ``seq``\ *n*\ ``<``\ |t1...tn| ``>``.

:Postcondition:
    ``size<s>::value == n``.


Models
------

* |vector|
* |list|
* |map|

See also
--------

|Sequences|, |Configuration|, |Integral Sequence Wrapper|

.. |variadic| replace:: `variadic`_
.. _`variadic`: `Variadic Sequence`_

.. |variadic forms| replace:: `variadic forms`_
.. _`variadic forms`: `Variadic Sequence`_

.. |numbered forms| replace:: `numbered forms`_
.. _`numbered forms`: `Variadic Sequence`_


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
