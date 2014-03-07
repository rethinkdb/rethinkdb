.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

++++++++++++++++++
 Iterator Concepts
++++++++++++++++++

:Author: David Abrahams, Jeremy Siek, Thomas Witt
:Contact: dave@boost-consulting.com, jsiek@osl.iu.edu, witt@styleadvisor.com
:organization: `Boost Consulting`_, Indiana University `Open Systems
               Lab`_, `Zephyr Associates, Inc.`_
:date: $Date: 2008-03-22 14:45:55 -0700 (Sat, 22 Mar 2008) $
:copyright: Copyright David Abrahams, Jeremy Siek, and Thomas Witt 2004. 

.. _`Boost Consulting`: http://www.boost-consulting.com
.. _`Open Systems Lab`: http://www.osl.iu.edu
.. _`Zephyr Associates, Inc.`: http://www.styleadvisor.com

:abstract:  The iterator concept checking classes provide a mechanism for
  a template to report better error messages when a user instantiates
  the template with a type that does not meet the requirements of
  the template.


For an introduction to using concept checking classes, see
the documentation for the |concepts|_ library.

.. |concepts| replace:: ``boost::concept_check``
.. _concepts: ../../concept_check/index.html


Reference
=========

Iterator Access Concepts
........................

* |Readable|_ 
* |Writable|_ 
* |Swappable|_ 
* |Lvalue|_ 

.. |Readable| replace:: *Readable Iterator*
.. _Readable: ReadableIterator.html

.. |Writable| replace:: *Writable Iterator*
.. _Writable: WritableIterator.html

.. |Swappable| replace:: *Swappable Iterator*
.. _Swappable: SwappableIterator.html

.. |Lvalue| replace:: *Lvalue Iterator*
.. _Lvalue: LvalueIterator.html


Iterator Traversal Concepts
...........................

* |Incrementable|_
* |SinglePass|_
* |Forward|_
* |Bidir|_
* |Random|_


.. |Incrementable| replace:: *Incrementable Iterator*
.. _Incrementable: IncrementableIterator.html

.. |SinglePass| replace:: *Single Pass Iterator*
.. _SinglePass: SinglePassIterator.html

.. |Forward| replace:: *Forward Traversal*
.. _Forward: ForwardTraversal.html

.. |Bidir| replace:: *Bidirectional Traversal*
.. _Bidir: BidirectionalTraversal.html

.. |Random| replace:: *Random Access Traversal*
.. _Random: RandomAccessTraversal.html



``iterator_concepts.hpp`` Synopsis
..................................

::

    namespace boost_concepts {

        // Iterator Access Concepts

        template <typename Iterator>
        class ReadableIteratorConcept;

        template <
            typename Iterator
          , typename ValueType = std::iterator_traits<Iterator>::value_type
        >
        class WritableIteratorConcept;

        template <typename Iterator>
        class SwappableIteratorConcept;

        template <typename Iterator>
        class LvalueIteratorConcept;

        // Iterator Traversal Concepts

        template <typename Iterator>
        class IncrementableIteratorConcept;

        template <typename Iterator>
        class SinglePassIteratorConcept;

        template <typename Iterator>
        class ForwardTraversalConcept;

        template <typename Iterator>
        class BidirectionalTraversalConcept;

        template <typename Iterator>
        class RandomAccessTraversalConcept;

        // Interoperability

        template <typename Iterator, typename ConstIterator>
        class InteroperableIteratorConcept;

    }
