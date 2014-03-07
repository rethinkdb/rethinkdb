.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

++++++++++++++++++++++++++++++++++++++++
 ``pointee`` and ``indirect_reference`` 
++++++++++++++++++++++++++++++++++++++++

:Author: David Abrahams
:Contact: dave@boost-consulting.com
:organization: `Boost Consulting`_
:date: $Date: 2008-03-22 14:45:55 -0700 (Sat, 22 Mar 2008) $
:copyright: Copyright David Abrahams 2004. 

.. _`Boost Consulting`: http://www.boost-consulting.com

:abstract: Provides the capability to deduce the referent types of
  pointers, smart pointers and iterators in generic code.

Overview
========

Have you ever wanted to write a generic function that can operate
on any kind of dereferenceable object?  If you have, you've
probably run into the problem of how to determine the type that the
object "points at":

.. parsed-literal::

   template <class Dereferenceable>
   void f(Dereferenceable p)
   {
       *what-goes-here?* value = \*p;
       ...
   }


``pointee``
-----------

It turns out to be impossible to come up with a fully-general
algorithm to do determine *what-goes-here* directly, but it is
possible to require that ``pointee<Dereferenceable>::type`` is
correct. Naturally, ``pointee`` has the same difficulty: it can't
determine the appropriate ``::type`` reliably for all
``Dereferenceable``\ s, but it makes very good guesses (it works
for all pointers, standard and boost smart pointers, and
iterators), and when it guesses wrongly, it can be specialized as
necessary::

  namespace boost
  {
    template <class T>
    struct pointee<third_party_lib::smart_pointer<T> >
    {
        typedef T type;
    };
  }

``indirect_reference``
----------------------

``indirect_reference<T>::type`` is rather more specialized than
``pointee``, and is meant to be used to forward the result of
dereferencing an object of its argument type.  Most dereferenceable
types just return a reference to their pointee, but some return
proxy references or return the pointee by value.  When that
information is needed, call on ``indirect_reference``.

Both of these templates are essential to the correct functioning of
|indirect_iterator|_.

.. |indirect_iterator| replace:: ``indirect_iterator``
.. _indirect_iterator: indirect_iterator.html

Reference
=========

``pointee``
-----------

.. include:: pointee_ref.rst

``indirect_reference``
----------------------

.. include:: indirect_reference_ref.rst

