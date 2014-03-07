.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. parsed-literal::

  template< class ElementIterator
	  , class IndexIterator
	  , class ValueT        = use_default
	  , class CategoryT     = use_default
	  , class ReferenceT    = use_default
	  , class DifferenceT   = use_default >
  class permutation_iterator
  {
  public:
    permutation_iterator();
    explicit permutation_iterator(ElementIterator x, IndexIterator y);

    template< class OEIter, class OIIter, class V, class C, class R, class D >
    permutation_iterator(
	permutation_iterator<OEIter, OIIter, V, C, R, D> const& r
	, typename enable_if_convertible<OEIter, ElementIterator>::type* = 0
	, typename enable_if_convertible<OIIter, IndexIterator>::type* = 0
	);
    reference operator*() const;
    permutation_iterator& operator++();
    ElementIterator const& base() const;
  private:
    ElementIterator m_elt;      // exposition only
    IndexIterator m_order;      // exposition only
  };

  template <class ElementIterator, class IndexIterator>
  permutation_iterator<ElementIterator, IndexIterator> 
  make_permutation_iterator( ElementIterator e, IndexIterator i);



``permutation_iterator`` requirements
-------------------------------------

``ElementIterator`` shall model Random Access Traversal Iterator.
``IndexIterator`` shall model Readable Iterator.  The value type of
the ``IndexIterator`` must be convertible to the difference type of
``ElementIterator``.


``permutation_iterator`` models
-------------------------------

``permutation_iterator`` models the same iterator traversal concepts
as ``IndexIterator`` and the same iterator access concepts as
``ElementIterator``.

If ``IndexIterator`` models Single Pass Iterator and 
``ElementIterator`` models Readable Iterator then
``permutation_iterator`` models Input Iterator.

If ``IndexIterator`` models Forward Traversal Iterator and 
``ElementIterator`` models Readable Lvalue Iterator then
``permutation_iterator`` models Forward Iterator.

If ``IndexIterator`` models Bidirectional Traversal Iterator and 
``ElementIterator`` models Readable Lvalue Iterator then
``permutation_iterator`` models Bidirectional Iterator.

If ``IndexIterator`` models Random Access Traversal Iterator and
``ElementIterator`` models Readable Lvalue Iterator then
``permutation_iterator`` models Random Access Iterator.

``permutation_iterator<E1, X, V1, C2, R1, D1>`` is interoperable
with ``permutation_iterator<E2, Y, V2, C2, R2, D2>`` if and only if
``X`` is interoperable with ``Y`` and ``E1`` is convertible
to ``E2``.


``permutation_iterator`` operations
-----------------------------------

In addition to those operations required by the concepts that
``permutation_iterator`` models, ``permutation_iterator`` provides the
following operations.

``permutation_iterator();``

:Effects: Default constructs ``m_elt`` and ``m_order``.


``explicit permutation_iterator(ElementIterator x, IndexIterator y);``

:Effects: Constructs ``m_elt`` from ``x`` and ``m_order`` from ``y``.


::

    template< class OEIter, class OIIter, class V, class C, class R, class D >
    permutation_iterator(
	permutation_iterator<OEIter, OIIter, V, C, R, D> const& r
	, typename enable_if_convertible<OEIter, ElementIterator>::type* = 0
	, typename enable_if_convertible<OIIter, IndexIterator>::type* = 0
	);

:Effects: Constructs ``m_elt`` from ``r.m_elt`` and
  ``m_order`` from ``y.m_order``.


``reference operator*() const;``

:Returns: ``*(m_elt + *m_order)``


``permutation_iterator& operator++();``

:Effects: ``++m_order``
:Returns: ``*this``


``ElementIterator const& base() const;``

:Returns: ``m_order``


::

  template <class ElementIterator, class IndexIterator>
  permutation_iterator<ElementIterator, IndexIterator> 
  make_permutation_iterator(ElementIterator e, IndexIterator i);

:Returns: ``permutation_iterator<ElementIterator, IndexIterator>(e, i)``

