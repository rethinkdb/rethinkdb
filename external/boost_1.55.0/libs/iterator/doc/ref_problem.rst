++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Problem with ``reference`` and old/new iterator category correspondance
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. _N1550: http://www.boost-consulting.com/writing/n1550.html
.. _N1530: http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/papers/2003/n1530.html

:Author: David Abrahams and Jeremy Siek
:Contact: dave@boost-consulting.com, jsiek@osl.iu.edu
:Organization: `Boost Consulting`_, Indiana University Bloomington
:date: $Date: 2003-11-17 08:52:29 -0800 (Mon, 17 Nov 2003) $
:Copyright: Copyright David Abrahams, Jeremy Siek 2003. Use, modification and
      distribution is subject to the Boost Software License,
      Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
      at http://www.boost.org/LICENSE_1_0.txt)

.. _`Boost Consulting`: http://www.boost-consulting.com

==============
 Introduction
==============

The new iterator categories are intended to correspond to the old
iterator categories, as specified in a diagram in N1550_. For example,
an iterator categorized as a mutable Forward Iterator under the old
scheme is now a Writable, Lvalue, and Foward Traversal iterator.
However, there is a problem with this correspondance, the new iterator
categories place requirements on the ``iterator_traits<X>::reference``
type whereas the standard iterator requirements say nothing about the
``reference`` type . In particular, the new Readable Iterator
requirements say that the return type of ``*a`` must be
``iterator_traits<X>::reference`` and the Lvalue Iterator requirements
says that ``iterator_traits<X>::reference`` must be ``T&`` or ``const
T&``.


====================
 Proposed Resolution
====================

Change the standard requirements to match the requirements of the new
iterators. (more details to come)


==========
 Rationale
==========

The lack of specification in the standard of the ``reference`` type is
certainly a defect. Without specification, it is entirely useless in a
generic function. The current practice in the community is generally
to assume there are requirements on the ``reference`` type, such as
those proposed in the new iterator categories.

There is some danger in *adding* requirements to existing concepts.
This will mean that some existing iterator types will no longer meet
the iterator requirements. However, we feel that the impact of this is
small enough to warrant going ahead with this change.

An alternative solution would be to leave the standard requirements as
is, and to remove the requirements for the ``reference`` type in the
new iterator concepts. We are not in favor of this approach because it
extends what we see as a defect further into the future.
