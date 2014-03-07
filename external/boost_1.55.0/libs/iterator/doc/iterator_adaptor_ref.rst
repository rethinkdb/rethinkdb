.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. Version 1.4 of this ReStructuredText document corresponds to
   n1530_, the paper accepted by the LWG for TR1.

.. Copyright David Abrahams, Jeremy Siek, and Thomas Witt 2003. 

.. parsed-literal::
  
  template <
      class Derived
    , class Base
    , class Value               = use_default
    , class CategoryOrTraversal = use_default
    , class Reference           = use_default
    , class Difference = use_default
  >
  class iterator_adaptor 
    : public iterator_facade<Derived, *V'*, *C'*, *R'*, *D'*> // see details__
  {
      friend class iterator_core_access;
   public:
      iterator_adaptor();
      explicit iterator_adaptor(Base const& iter);
      typedef Base base_type;
      Base const& base() const;
   protected:
      typedef iterator_adaptor iterator_adaptor\_;
      Base const& base_reference() const;
      Base& base_reference();
   private: // Core iterator interface for iterator_facade.  
      typename iterator_adaptor::reference dereference() const;

      template <
      class OtherDerived, class OtherIterator, class V, class C, class R, class D
      >   
      bool equal(iterator_adaptor<OtherDerived, OtherIterator, V, C, R, D> const& x) const;
  
      void advance(typename iterator_adaptor::difference_type n);
      void increment();
      void decrement();

      template <
          class OtherDerived, class OtherIterator, class V, class C, class R, class D
      >   
      typename iterator_adaptor::difference_type distance_to(
          iterator_adaptor<OtherDerived, OtherIterator, V, C, R, D> const& y) const;

   private:
      Base m_iterator; // exposition only
  };

__ base_parameters_

.. _requirements:

``iterator_adaptor`` requirements
---------------------------------

``static_cast<Derived*>(iterator_adaptor*)`` shall be well-formed.
The ``Base`` argument shall be Assignable and Copy Constructible.


.. _base_parameters:

``iterator_adaptor`` base class parameters
------------------------------------------

The *V'*, *C'*, *R'*, and *D'* parameters of the ``iterator_facade``
used as a base class in the summary of ``iterator_adaptor``
above are defined as follows:

.. parsed-literal::

   *V'* = if (Value is use_default)
             return iterator_traits<Base>::value_type
         else
             return Value

   *C'* = if (CategoryOrTraversal is use_default)
             return iterator_traversal<Base>::type
         else
             return CategoryOrTraversal

   *R'* = if (Reference is use_default)
             if (Value is use_default)
                 return iterator_traits<Base>::reference
             else
                 return Value&
         else
             return Reference

   *D'* = if (Difference is use_default)
             return iterator_traits<Base>::difference_type
         else
             return Difference

.. ``iterator_adaptor`` models
   ---------------------------

   In order for ``Derived`` to model the iterator concepts corresponding
   to ``iterator_traits<Derived>::iterator_category``, the expressions
   involving ``m_iterator`` in the specifications of those private member
   functions of ``iterator_adaptor`` that may be called by
   ``iterator_facade<Derived, V, C, R, D>`` in evaluating any valid
   expression involving ``Derived`` in those concepts' requirements.

.. The above is confusing and needs a rewrite. -JGS
.. That's why it's removed.  We're embracing inheritance, remember?

``iterator_adaptor`` public operations
--------------------------------------

``iterator_adaptor();``

:Requires: The ``Base`` type must be Default Constructible.
:Returns: An instance of ``iterator_adaptor`` with 
    ``m_iterator`` default constructed.


``explicit iterator_adaptor(Base const& iter);``

:Returns: An instance of ``iterator_adaptor`` with
    ``m_iterator`` copy constructed from ``iter``.

``Base const& base() const;``

:Returns: ``m_iterator``

``iterator_adaptor`` protected member functions
-----------------------------------------------

``Base const& base_reference() const;``

:Returns: A const reference to ``m_iterator``.


``Base& base_reference();``

:Returns: A non-const reference to ``m_iterator``.


``iterator_adaptor`` private member functions
---------------------------------------------

``typename iterator_adaptor::reference dereference() const;``

:Returns: ``*m_iterator``

::

  template <
  class OtherDerived, class OtherIterator, class V, class C, class R, class D
  >   
  bool equal(iterator_adaptor<OtherDerived, OtherIterator, V, C, R, D> const& x) const;

:Returns: ``m_iterator == x.base()``


``void advance(typename iterator_adaptor::difference_type n);``

:Effects: ``m_iterator += n;``

``void increment();``

:Effects: ``++m_iterator;``

``void decrement();``

:Effects: ``--m_iterator;``

::

  template <
      class OtherDerived, class OtherIterator, class V, class C, class R, class D
  >   
  typename iterator_adaptor::difference_type distance_to(
      iterator_adaptor<OtherDerived, OtherIterator, V, C, R, D> const& y) const;

:Returns: ``y.base() - m_iterator``
