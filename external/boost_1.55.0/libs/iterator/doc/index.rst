.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

+++++++++++++++++++++++++++++++++++++++++++++++++
 The Boost.Iterator Library |(logo)|__
+++++++++++++++++++++++++++++++++++++++++++++++++

.. |(logo)| image:: ../../../boost.png
   :alt: Boost

__ ../../../index.htm


-------------------------------------


:Authors:       David Abrahams, Jeremy Siek, Thomas Witt
:Contact:       dave@boost-consulting.com, jsiek@osl.iu.edu, witt@styleadvisor.com
:organizations: `Boost Consulting`_, Indiana University `Open Systems
                Lab`_, `Zephyr Associates, Inc.`_
:date:          $Date: 2011-03-29 14:17:11 -0700 (Tue, 29 Mar 2011) $

:copyright:     Copyright David Abrahams, Jeremy Siek, Thomas Witt 2003.

.. _`Boost Consulting`: http://www.boost-consulting.com
.. _`Open Systems Lab`: http://www.osl.iu.edu
.. _`Zephyr Associates, Inc.`: http://www.styleadvisor.com

:Abstract: The Boost Iterator Library contains two parts. The first
           is a system of concepts_ which extend the C++ standard
           iterator requirements. The second is a framework of
           components for building iterators based on these
           extended concepts and includes several useful iterator
           adaptors. The extended iterator concepts have been
           carefully designed so that old-style iterators
           can fit in the new concepts and so that new-style
           iterators will be compatible with old-style algorithms,
           though algorithms may need to be updated if they want to
           take full advantage of the new-style iterator
           capabilities.  Several components of this library have
           been accepted into the C++ standard technical report.
           The components of the Boost Iterator Library replace the
           older Boost Iterator Adaptor Library.

.. _concepts: http://www.boost.org/more/generic_programming.html#concept

.. contents:: **Table of Contents**


-------------------------------------


=====================
 New-Style Iterators
=====================

The iterator categories defined in C++98 are extremely limiting
because they bind together two orthogonal concepts: traversal and
element access.  For example, because a random access iterator is
required to return a reference (and not a proxy) when dereferenced,
it is impossible to capture the capabilities of
``vector<bool>::iterator`` using the C++98 categories.  This is the
infamous "``vector<bool>`` is not a container, and its iterators
aren't random access iterators", debacle about which Herb Sutter
wrote two papers for the standards comittee (n1185_ and n1211_),
and a `Guru of the Week`__.  New-style iterators go well beyond
patching up ``vector<bool>``, though: there are lots of other
iterators already in use which can't be adequately represented by
the existing concepts.  For details about the new iterator
concepts, see our

.. _n1185: http://www.gotw.ca/publications/N1185.pdf
.. _n1211: http://www.gotw.ca/publications/N1211.pdf
__ http://www.gotw.ca/gotw/050.htm


   `Standard Proposal For New-Style Iterators`__ (PDF__)

__ new-iter-concepts.html
__ new-iter-concepts.pdf

=============================
 Iterator Facade and Adaptor
=============================

Writing standard-conforming iterators is tricky, but the need comes
up often.  In order to ease the implementation of new iterators,
the Boost.Iterator library provides the |facade| class template,
which implements many useful defaults and compile-time checks
designed to help the iterator author ensure that his iterator is
correct.  

It is also common to define a new iterator that is similar to some
underlying iterator or iterator-like type, but that modifies some
aspect of the underlying type's behavior.  For that purpose, the
library supplies the |adaptor| class template, which is specially
designed to take advantage of as much of the underlying type's
behavior as possible.

The documentation for these two classes can be found at the following
web pages:

* |facade|_ (PDF__)

* |adaptor|_ (PDF__)


.. |facade| replace:: ``iterator_facade``
.. _facade: iterator_facade.html
__ iterator_facade.pdf

.. |adaptor| replace:: ``iterator_adaptor``
.. _adaptor: iterator_adaptor.html
__ iterator_adaptor.pdf

Both |facade| and |adaptor| as well as many of the `specialized
adaptors`_ mentioned below have been proposed for standardization,
and accepted into the first C++ technical report; see our

   `Standard Proposal For Iterator Facade and Adaptor`__ (PDF__)

for more details.

__ facade-and-adaptor.html
__ facade-and-adaptor.pdf

======================
 Specialized Adaptors
======================

The iterator library supplies a useful suite of standard-conforming
iterator templates based on the Boost `iterator facade and adaptor`_.

* |counting|_ (PDF__): an iterator over a sequence of consecutive values.
  Implements a "lazy sequence"

* |filter|_ (PDF__): an iterator over the subset of elements of some
  sequence which satisfy a given predicate

* |function_input|_ (PDF__): an input iterator wrapping a generator (nullary
  function object); each time the iterator is dereferenced, the function object
  is called to get the value to return.

* |function_output|_ (PDF__): an output iterator wrapping a unary function
  object; each time an element is written into the dereferenced
  iterator, it is passed as a parameter to the function object.

* |indirect|_ (PDF__): an iterator over the objects *pointed-to* by the
  elements of some sequence.

* |permutation|_ (PDF__): an iterator over the elements of some random-access
  sequence, rearranged according to some sequence of integer indices.

* |reverse|_ (PDF__): an iterator which traverses the elements of some
  bidirectional sequence in reverse.  Corrects many of the
  shortcomings of C++98's ``std::reverse_iterator``.

* |shared|_: an iterator over elements of a container whose
  lifetime is maintained by a |shared_ptr|_ stored in the iterator.

* |transform|_ (PDF__): an iterator over elements which are the result of
  applying some functional transformation to the elements of an
  underlying sequence.  This component also replaces the old
  ``projection_iterator_adaptor``.

* |zip|_ (PDF__): an iterator over tuples of the elements at corresponding
  positions of heterogeneous underlying iterators.

.. |counting| replace:: ``counting_iterator``
.. _counting: counting_iterator.html
__ counting_iterator.pdf

.. |filter| replace:: ``filter_iterator``
.. _filter: filter_iterator.html
__ filter_iterator.pdf

.. |function_input| replace:: ``function_input_iterator``
.. _function_input: function_input_iterator.html
__ function_input_iterator.pdf

.. |function_output| replace:: ``function_output_iterator``
.. _function_output: function_output_iterator.html
__ function_output_iterator.pdf

.. |indirect| replace:: ``indirect_iterator``
.. _indirect: indirect_iterator.html
__ indirect_iterator.pdf

.. |permutation| replace:: ``permutation_iterator``
.. _permutation: permutation_iterator.html
__ permutation_iterator.pdf

.. |reverse| replace:: ``reverse_iterator``
.. _reverse: reverse_iterator.html
__ reverse_iterator.pdf

.. |shared| replace:: ``shared_container_iterator``
.. _shared: ../../utility/shared_container_iterator.html

.. |transform| replace:: ``transform_iterator``
.. _transform: transform_iterator.html
__ transform_iterator.pdf

.. |zip| replace:: ``zip_iterator``
.. _zip: zip_iterator.html
__ zip_iterator.pdf

.. |shared_ptr| replace:: ``shared_ptr``
.. _shared_ptr: ../../smart_ptr/shared_ptr.htm

====================
 Iterator Utilities
====================

Traits
------

* |pointee|_ (PDF__): Provides the capability to deduce the referent types
  of pointers, smart pointers and iterators in generic code.  Used
  in |indirect|.

* |iterator_traits|_ (PDF__): Provides MPL_\ -compatible metafunctions which
  retrieve an iterator's traits.  Also corrects for the deficiencies
  of broken implementations of ``std::iterator_traits``.

.. * |interoperable|_ (PDF__): Provides an MPL_\ -compatible metafunction for
     testing iterator interoperability

.. |pointee| replace:: ``pointee.hpp``
.. _pointee: pointee.html
__ pointee.pdf

.. |iterator_traits| replace:: ``iterator_traits.hpp``
.. _iterator_traits: iterator_traits.html
__ iterator_traits.pdf

.. |interoperable| replace:: ``interoperable.hpp``
.. _interoperable: interoperable.html
.. comment! __ interoperable.pdf

.. _MPL: ../../mpl/doc/index.html

Testing and Concept Checking
----------------------------

* |iterator_concepts|_ (PDF__): Concept checking classes for the new iterator concepts.

* |iterator_archetypes|_ (PDF__): Concept archetype classes for the new iterators concepts.

.. |iterator_concepts| replace:: ``iterator_concepts.hpp``
.. _iterator_concepts: iterator_concepts.html
__ iterator_concepts.pdf

.. |iterator_archetypes| replace:: ``iterator_archetypes.hpp``
.. _iterator_archetypes: iterator_archetypes.html
__ iterator_archetypes.pdf

=======================================================
 Upgrading from the old Boost Iterator Adaptor Library
=======================================================

.. _Upgrading:

If you have been using the old Boost Iterator Adaptor library to
implement iterators, you probably wrote a ``Policies`` class which
captures the core operations of your iterator.  In the new library
design, you'll move those same core operations into the body of the
iterator class itself.  If you were writing a family of iterators,
you probably wrote a `type generator`_ to build the
``iterator_adaptor`` specialization you needed; in the new library
design you don't need a type generator (though may want to keep it
around as a compatibility aid for older code) because, due to the
use of the Curiously Recurring Template Pattern (CRTP) [Cop95]_,
you can now define the iterator class yourself and acquire
functionality through inheritance from ``iterator_facade`` or
``iterator_adaptor``.  As a result, you also get much finer control
over how your iterator works: you can add additional constructors,
or even override the iterator functionality provided by the
library.

.. _`type generator`: http://www.boost.org/more/generic_programming.html#type_generator

If you're looking for the old ``projection_iterator`` component,
its functionality has been merged into ``transform_iterator``: as
long as the function object's ``result_type`` (or the ``Reference``
template argument, if explicitly specified) is a true reference
type, ``transform_iterator`` will behave like
``projection_iterator`` used to.

=========
 History
=========

In 2000 Dave Abrahams was writing an iterator for a container of
pointers, which would access the pointed-to elements when
dereferenced.  Naturally, being a library writer, he decided to
generalize the idea and the Boost Iterator Adaptor library was born.
Dave was inspired by some writings of Andrei Alexandrescu and chose a
policy based design (though he probably didn't capture Andrei's idea
very well - there was only one policy class for all the iterator's
orthogonal properties).  Soon Jeremy Siek realized he would need the
library and they worked together to produce a "Boostified" version,
which was reviewed and accepted into the library.  They wrote a paper
and made several important revisions of the code.

Eventually, several shortcomings of the older library began to make
the need for a rewrite apparent.  Dave and Jeremy started working
at the Santa Cruz C++ committee meeting in 2002, and had quickly
generated a working prototype.  At the urging of Mat Marcus, they
decided to use the GenVoca/CRTP pattern approach, and moved the
policies into the iterator class itself.  Thomas Witt expressed
interest and became the voice of strict compile-time checking for
the project, adding uses of the SFINAE technique to eliminate false
converting constructors and operators from the overload set.  He
also recognized the need for a separate ``iterator_facade``, and
factored it out of ``iterator_adaptor``.  Finally, after a
near-complete rewrite of the prototype, they came up with the
library you see today.

.. [Cop95] [Coplien, 1995] Coplien, J., Curiously Recurring Template
   Patterns, C++ Report, February 1995, pp. 24-27.

..
 LocalWords:  Abrahams Siek Witt const bool Sutter's WG int UL LI href Lvalue
 LocalWords:  ReadableIterator WritableIterator SwappableIterator cv pre iter
 LocalWords:  ConstantLvalueIterator MutableLvalueIterator CopyConstructible TR
 LocalWords:  ForwardTraversalIterator BidirectionalTraversalIterator lvalue
 LocalWords:  RandomAccessTraversalIterator dereferenceable Incrementable tmp
 LocalWords:  incrementable xxx min prev inplace png oldeqnew AccessTag struct
 LocalWords:  TraversalTag typename lvalues DWA Hmm JGS
