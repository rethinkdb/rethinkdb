++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Problem with ``is_writable`` and ``is_swappable`` in N1550_
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. _N1550: http://www.boost-consulting.com/writing/n1550.html
.. _N1530: http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/papers/2003/n1530.html

:Author: David Abrahams and Jeremy Siek
:Contact: dave@boost-consulting.com, jsiek@osl.iu.edu
:Organization: `Boost Consulting`_, Indiana University Bloomington
:date: $Date: 2008-03-22 14:45:55 -0700 (Sat, 22 Mar 2008) $
:Copyright: Copyright David Abrahams, Jeremy Siek 2003. Use, modification and
      distribution is subject to the Boost Software License,
      Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
      at http://www.boost.org/LICENSE_1_0.txt)

.. _`Boost Consulting`: http://www.boost-consulting.com

.. contents:: Table of Contents

==============
 Introduction
==============

The ``is_writable`` and ``is_swappable`` traits classes in N1550_
provide a mechanism for determining at compile time if an iterator
type is a model of the new Writable Iterator and Swappable Iterator
concepts, analogous to ``iterator_traits<X>::iterator_category``
for the old iterator concepts. For backward compatibility,
``is_writable`` and ``is_swappable`` not only work with new
iterators, but they also are intended to work for old
iterators (iterators that meet the requirements for one of the
iterator concepts in the current standard). In the case of old
iterators, the writability and swapability is deduced based on the
``iterator_category`` and also the ``reference`` type. The
specification for this deduction gives false positives for forward
iterators that have non-assignable value types.

To review, the part of the ``is_writable`` trait definition which
applies to old iterators is::

  if (cat is convertible to output_iterator_tag)
      return true;
  else if (cat is convertible to forward_iterator_tag
           and iterator_traits<Iterator>::reference is a 
               mutable reference)
      return true;
  else
      return false;

Suppose the ``value_type`` of the iterator ``It`` has a private
assignment operator::

  class B {
  public:
    ...
  private:
    B& operator=(const B&);
  };

and suppose the ``reference`` type of the iterator is ``B&``.  In
that case, ``is_writable<It>::value`` will be true when in fact
attempting to write into ``B`` will cause an error.

The same problem applies to ``is_swappable``.


====================
 Proposed Resolution
====================

1. Remove the ``is_writable`` and ``is_swappable`` traits, and remove the
   requirements in the Writable Iterator and Swappable Iterator concepts
   that require their models to support these traits.

2. Change the ``is_readable`` specification to be:
   ``is_readable<X>::type`` is ``true_type`` if the
   result type of ``X::operator*`` is convertible to
   ``iterator_traits<X>::value_type`` and is ``false_type``
   otherwise. Also, ``is_readable`` is required to satisfy
   the requirements for the UnaryTypeTrait concept
   (defined in the type traits proposal).
   
   Remove the requirement for support of the ``is_readable`` trait from
   the Readable Iterator concept.


3. Remove the ``iterator_tag`` class.

4. Change the specification of ``traversal_category`` to::

    traversal-category(Iterator) =
        let cat = iterator_traits<Iterator>::iterator_category
        if (cat is convertible to incrementable_iterator_tag)
          return cat; // Iterator is a new iterator
        else if (cat is convertible to random_access_iterator_tag)
            return random_access_traversal_tag;
        else if (cat is convertible to bidirectional_iterator_tag)
            return bidirectional_traversal_tag;
        else if (cat is convertible to forward_iterator_tag)
            return forward_traversal_tag;
        else if (cat is convertible to input_iterator_tag)
            return single_pass_iterator_tag;
        else if (cat is convertible to output_iterator_tag)
            return incrementable_iterator_tag;
        else
            return null_category_tag;


==========
 Rationale
==========

1. There are two reasons for removing ``is_writable``
   and ``is_swappable``. The first is that we do not know of
   a way to fix the specification so that it gives the correct
   answer for all iterators. Second, there was only a weak
   motivation for having ``is_writable`` and ``is_swappable``
   there in the first place.  The main motivation was simply
   uniformity: we have tags for the old iterator categories
   so we should have tags for the new iterator categories.
   While having tags and the capability to dispatch based
   on the traversal categories is often used, we see
   less of a need for dispatching based on writability
   and swappability, since typically algorithms
   that need these capabilities have no alternative if
   they are not provided.

2. We discovered that the ``is_readable`` trait can be implemented
   using only the iterator type itself and its ``value_type``.
   Therefore we remove the requirement for ``is_readable`` from the
   Readable Iterator concept, and change the definition of
   ``is_readable`` so that it works for any iterator type.

3. The purpose of the ``iterator_tag`` class was to
   bundle the traversal and access category tags
   into the ``iterator_category`` typedef.
   With ``is_writable`` and ``is_swappable`` gone, and
   ``is_readable`` no longer in need of special hints,
   there is no reason for iterators to provide
   information about the access capabilities of an iterator.
   Thus there is no need for the ``iterator_tag``. The
   traversal tag can be directly used for the
   ``iterator_category``. If a new iterator is intended to be backward
   compatible with old iterator concepts, a tag type
   that is convertible to both one of the new traversal tags 
   and also to an old iterator tag can be created and use
   for the ``iterator_category``.

4. The changes to the specification of ``traversal_category`` are a 
   direct result of the removal of ``iterator_tag``.

