.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

+++++++++++++++++++++++++++++
 Iterator Facade and Adaptor
+++++++++++++++++++++++++++++

:Author: David Abrahams, Jeremy Siek, Thomas Witt
:Contact: dave@boost-consulting.com, jsiek@osl.iu.edu, witt@styleadvisor.com
:organization: `Boost Consulting`_, Indiana University `Open Systems
               Lab`_, `Zephyr Associates, Inc.`_
:date: $Date: 2006-09-11 15:27:29 -0700 (Mon, 11 Sep 2006) $

:Number: This is a revised version of N1530_\ =03-0113, which was
         accepted for Technical Report 1 by the C++ standard
         committee's library working group.  

.. Version 1.9 of this ReStructuredText document corresponds to
   n1530_, the paper accepted by the LWG.

.. _n1530: http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/papers/2003/n1530.html

:copyright: Copyright David Abrahams, Jeremy Siek, and Thomas Witt 2003. 

.. _`Boost Consulting`: http://www.boost-consulting.com
.. _`Open Systems Lab`: http://www.osl.iu.edu
.. _`Zephyr Associates, Inc.`: http://www.styleadvisor.com

:abstract: We propose a set of class templates that help programmers
           build standard-conforming iterators, both from scratch and
           by adapting other iterators.

.. contents:: Table of Contents

============
 Motivation
============

Iterators play an important role in modern C++ programming. The
iterator is the central abstraction of the algorithms of the Standard
Library, allowing algorithms to be re-used in in a wide variety of
contexts.  The C++ Standard Library contains a wide variety of useful
iterators. Every one of the standard containers comes with constant
and mutable iterators [#mutable]_, and also reverse versions of those
same iterators which traverse the container in the opposite direction.
The Standard also supplies ``istream_iterator`` and
``ostream_iterator`` for reading from and writing to streams,
``insert_iterator``, ``front_insert_iterator`` and
``back_insert_iterator`` for inserting elements into containers, and
``raw_storage_iterator`` for initializing raw memory [7].

Despite the many iterators supplied by the Standard Library, obvious
and useful iterators are missing, and creating new iterator types is
still a common task for C++ programmers.  The literature documents
several of these, for example line_iterator [3] and Constant_iterator
[9].  The iterator abstraction is so powerful that we expect
programmers will always need to invent new iterator types.

Although it is easy to create iterators that *almost* conform to the
standard, the iterator requirements contain subtleties which can make
creating an iterator which *actually* conforms quite difficult.
Further, the iterator interface is rich, containing many operators
that are technically redundant and tedious to implement.  To automate
the repetitive work of constructing iterators, we propose
``iterator_facade``, an iterator base class template which provides
the rich interface of standard iterators and delegates its
implementation to member functions of the derived class.  In addition
to reducing the amount of code necessary to create an iterator, the
``iterator_facade`` also provides compile-time error detection.
Iterator implementation mistakes that often go unnoticed are turned
into compile-time errors because the derived class implementation must
match the expectations of the ``iterator_facade``.

A common pattern of iterator construction is the adaptation of one
iterator to form a new one.  The functionality of an iterator is
composed of four orthogonal aspects: traversal, indirection, equality
comparison and distance measurement.  Adapting an old iterator to
create a new one often saves work because one can reuse one aspect of
functionality while redefining the other.  For example, the Standard
provides ``reverse_iterator``, which adapts any Bidirectional Iterator
by inverting its direction of traversal.  As with plain iterators,
iterator adaptors defined outside the Standard have become commonplace
in the literature:

* Checked iter[13] adds bounds-checking to an existing iterator.

* The iterators of the View Template Library[14], which adapts
  containers, are themselves adaptors over the underlying iterators.

* Smart iterators [5] adapt an iterator's dereferencing behavior by
  applying a function object to the object being referenced and
  returning the result.

* Custom iterators [4], in which a variety of adaptor types are enumerated.

* Compound iterators [1], which access a slice out of a container of containers.

* Several iterator adaptors from the MTL [12].  The MTL contains a
  strided iterator, where each call to ``operator++()`` moves the
  iterator ahead by some constant factor, and a scaled iterator, which
  multiplies the dereferenced value by some constant.

.. [#concept] We use the term concept to mean a set of requirements
   that a type must satisfy to be used with a particular template
   parameter.

.. [#mutable] The term mutable iterator refers to iterators over objects that
   can be changed by assigning to the dereferenced iterator, while
   constant iterator refers to iterators over objects that cannot be
   modified.

To fulfill the need for constructing adaptors, we propose the
``iterator_adaptor`` class template.  Instantiations of
``iterator_adaptor`` serve as a base classes for new iterators,
providing the default behavior of forwarding all operations to the
underlying iterator.  The user can selectively replace these features
in the derived iterator class.  This proposal also includes a number
of more specialized adaptors, such as the ``transform_iterator`` that
applies some user-specified function during the dereference of the
iterator.

========================
 Impact on the Standard
========================

This proposal is purely an addition to the C++ standard library.
However, note that this proposal relies on the proposal for New
Iterator Concepts.

========
 Design
========

Iterator Concepts
=================

This proposal is formulated in terms of the new ``iterator concepts``
as proposed in n1550_, since user-defined and especially adapted
iterators suffer from the well known categorization problems that are
inherent to the current iterator categories.

.. _n1550: http://anubis.dkuug.dk/JTC1/SC22/WG21/docs/papers/2003/n1550.html

This proposal does not strictly depend on proposal n1550_, as there
is a direct mapping between new and old categories. This proposal
could be reformulated using this mapping if n1550_ was not accepted.

Interoperability
================

The question of iterator interoperability is poorly addressed in the
current standard.  There are currently two defect reports that are
concerned with interoperability issues.

Issue 179_ concerns the fact that mutable container iterator types
are only required to be convertible to the corresponding constant
iterator types, but objects of these types are not required to
interoperate in comparison or subtraction expressions.  This situation
is tedious in practice and out of line with the way built in types
work.  This proposal implements the proposed resolution to issue
179_, as most standard library implementations do nowadays. In other
words, if an iterator type A has an implicit or user defined
conversion to an iterator type B, the iterator types are interoperable
and the usual set of operators are available.

Issue 280_ concerns the current lack of interoperability between
reverse iterator types. The proposed new reverse_iterator template
fixes the issues raised in 280. It provides the desired
interoperability without introducing unwanted overloads.

.. _179: http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/lwg-defects.html#179
.. _280: http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/lwg-active.html#280


Iterator Facade
===============

.. include:: iterator_facade_body.rst

Iterator Adaptor
================

.. include:: iterator_adaptor_body.rst

Specialized Adaptors
====================

This proposal also contains several examples of specialized adaptors
which were easily implemented using ``iterator_adaptor``:

* ``indirect_iterator``, which iterates over iterators, pointers,
  or smart pointers and applies an extra level of dereferencing.

* A new ``reverse_iterator``, which inverts the direction of a Base
  iterator's motion, while allowing adapted constant and mutable
  iterators to interact in the expected ways (unlike those in most
  implementations of C++98).

* ``transform_iterator``, which applies a user-defined function object
  to the underlying values when dereferenced.

* ``filter_iterator``, which provides a view of an iterator range in
  which some elements of the underlying range are skipped.

.. _counting: 

* ``counting_iterator``, which adapts any incrementable type
  (e.g. integers, iterators) so that incrementing/decrementing the
  adapted iterator and dereferencing it produces successive values of
  the Base type.

* ``function_output_iterator``, which makes it easier to create custom
  output iterators.

Based on examples in the Boost library, users have generated many new
adaptors, among them a permutation adaptor which applies some
permutation to a random access iterator, and a strided adaptor, which
adapts a random access iterator by multiplying its unit of motion by a
constant factor.  In addition, the Boost Graph Library (BGL) uses
iterator adaptors to adapt other graph libraries, such as LEDA [10]
and Stanford GraphBase [8], to the BGL interface (which requires C++
Standard compliant iterators).

===============
 Proposed Text
===============


Header ``<iterator_helper>`` synopsis    [lib.iterator.helper.synopsis]
=======================================================================


::

  struct use_default;

  struct iterator_core_access { /* implementation detail */ };
  
  template <
      class Derived
    , class Value
    , class CategoryOrTraversal
    , class Reference  = Value&
    , class Difference = ptrdiff_t
  >
  class iterator_facade;

  template <
      class Derived
    , class Base
    , class Value      = use_default
    , class CategoryOrTraversal  = use_default
    , class Reference  = use_default
    , class Difference = use_default
  >
  class iterator_adaptor;
  
  template <
      class Iterator
    , class Value = use_default
    , class CategoryOrTraversal = use_default
    , class Reference = use_default
    , class Difference = use_default
  >
  class indirect_iterator;
  
  template <class Dereferenceable>
  struct pointee;

  template <class Dereferenceable>
  struct indirect_reference;

  template <class Iterator>
  class reverse_iterator;

  template <
      class UnaryFunction
    , class Iterator
    , class Reference = use_default
    , class Value = use_default
  >
  class transform_iterator;

  template <class Predicate, class Iterator>
  class filter_iterator;

  template <
      class Incrementable
    , class CategoryOrTraversal  = use_default
    , class Difference = use_default
  >
  class counting_iterator;

  template <class UnaryFunction>
  class function_output_iterator;



Iterator facade [lib.iterator.facade]
=====================================

.. include:: iterator_facade_abstract.rst

Class template ``iterator_facade``
----------------------------------

.. include:: iterator_facade_ref.rst

Iterator adaptor [lib.iterator.adaptor]
=======================================

.. include:: iterator_adaptor_abstract.rst

Class template ``iterator_adaptor``
-----------------------------------

.. include:: iterator_adaptor_ref.rst


Specialized adaptors [lib.iterator.special.adaptors]
====================================================


The ``enable_if_convertible<X,Y>::type`` expression used in
this section is for exposition purposes. The converting constructors
for specialized adaptors should be only be in an overload set provided
that an object of type ``X`` is implicitly convertible to an object of
type ``Y``.  
The signatures involving ``enable_if_convertible`` should behave
*as-if* ``enable_if_convertible`` were defined to be::

  template <bool> enable_if_convertible_impl
  {};

  template <> enable_if_convertible_impl<true>
  { struct type; };

  template<typename From, typename To>
  struct enable_if_convertible
    : enable_if_convertible_impl<is_convertible<From,To>::value>
  {};

If an expression other than the default argument is used to supply
the value of a function parameter whose type is written in terms
of ``enable_if_convertible``, the program is ill-formed, no
diagnostic required.

[*Note:* The ``enable_if_convertible`` approach uses SFINAE to
take the constructor out of the overload set when the types are not
implicitly convertible.  
]


Indirect iterator
-----------------

.. include:: indirect_iterator_abstract.rst

Class template ``pointee``
....................................

.. include:: pointee_ref.rst

Class template ``indirect_reference``
.....................................

.. include:: indirect_reference_ref.rst

Class template ``indirect_iterator``
....................................

.. include:: indirect_iterator_ref.rst

Reverse iterator
----------------

.. include:: reverse_iterator_abstract.rst

Class template ``reverse_iterator``
...................................

.. include:: reverse_iterator_ref.rst


Transform iterator
------------------

.. include:: transform_iterator_abstract.rst

Class template ``transform_iterator``
.....................................

.. include:: transform_iterator_ref.rst


Filter iterator
---------------

.. include:: filter_iterator_abstract.rst


Class template ``filter_iterator``
..................................

.. include:: filter_iterator_ref.rst


Counting iterator
-----------------

.. include:: counting_iterator_abstract.rst

Class template ``counting_iterator``
....................................

.. include:: counting_iterator_ref.rst


Function output iterator
------------------------

.. include:: func_output_iter_abstract.rst

Class template ``function_output_iterator``
...........................................

.. include:: func_output_iter_ref.rst




.. LocalWords:  Abrahams Siek Witt istream ostream iter MTL strided interoperate
   LocalWords:  CRTP metafunctions inlining lvalue JGS incrementable BGL LEDA cv
   LocalWords:  GraphBase struct ptrdiff UnaryFunction const int typename bool pp
   LocalWords:  lhs rhs SFINAE markup iff tmp OtherDerived OtherIterator DWA foo
   LocalWords:  dereferenceable subobject AdaptableUnaryFunction impl pre ifdef'd
   LocalWords:  OtherIncrementable Coplien
