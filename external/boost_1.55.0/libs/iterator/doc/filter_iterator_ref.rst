.. Copyright David Abrahams, Jeremy Siek, and Thomas Witt
.. 2004. Use, modification and distribution is subject to the Boost
.. Software License, Version 1.0. (See accompanying  file
.. LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt) 

::

  template <class Predicate, class Iterator>
  class filter_iterator
  {
   public:
      typedef iterator_traits<Iterator>::value_type value_type;
      typedef iterator_traits<Iterator>::reference reference;
      typedef iterator_traits<Iterator>::pointer pointer;
      typedef iterator_traits<Iterator>::difference_type difference_type;
      typedef /* see below */ iterator_category;

      filter_iterator();
      filter_iterator(Predicate f, Iterator x, Iterator end = Iterator());
      filter_iterator(Iterator x, Iterator end = Iterator());
      template<class OtherIterator>
      filter_iterator(
          filter_iterator<Predicate, OtherIterator> const& t
          , typename enable_if_convertible<OtherIterator, Iterator>::type* = 0 // exposition
          );
      Predicate predicate() const;
      Iterator end() const;
      Iterator const& base() const;
      reference operator*() const;
      filter_iterator& operator++();
  private:
      Predicate m_pred; // exposition only
      Iterator m_iter;  // exposition only
      Iterator m_end;   // exposition only
  };


If ``Iterator`` models Readable Lvalue Iterator and Bidirectional Traversal
Iterator then ``iterator_category`` is convertible to
``std::bidirectional_iterator_tag``. 
Otherwise, if ``Iterator`` models Readable Lvalue Iterator and Forward Traversal
Iterator then ``iterator_category`` is convertible to
``std::forward_iterator_tag``. 
Otherwise ``iterator_category`` is
convertible to ``std::input_iterator_tag``.


``filter_iterator`` requirements
................................

The ``Iterator`` argument shall meet the requirements of Readable
Iterator and Single Pass Iterator or it shall meet the requirements of
Input Iterator.

The ``Predicate`` argument must be Assignable, Copy Constructible, and
the expression ``p(x)`` must be valid where ``p`` is an object of type
``Predicate``, ``x`` is an object of type
``iterator_traits<Iterator>::value_type``, and where the type of
``p(x)`` must be convertible to ``bool``.


``filter_iterator`` models
..........................

The concepts that ``filter_iterator`` models are dependent on which
concepts the ``Iterator`` argument models, as specified in the
following tables.

+---------------------------------+------------------------------------------+
|If ``Iterator`` models           |then ``filter_iterator`` models           |
+=================================+==========================================+
|Single Pass Iterator             |Single Pass Iterator                      |
+---------------------------------+------------------------------------------+
|Forward Traversal Iterator       |Forward Traversal Iterator                |
+---------------------------------+------------------------------------------+
|Bidirectional Traversal Iterator |Bidirectional Traversal Iterator          |
+---------------------------------+------------------------------------------+

+--------------------------------+----------------------------------------------+
| If ``Iterator`` models         | then ``filter_iterator`` models              |
+================================+==============================================+
| Readable Iterator              | Readable Iterator                            |
+--------------------------------+----------------------------------------------+
| Writable Iterator              | Writable Iterator                            |
+--------------------------------+----------------------------------------------+
| Lvalue Iterator                | Lvalue Iterator                              |
+--------------------------------+----------------------------------------------+

+-------------------------------------------------------+---------------------------------+
|If ``Iterator`` models                                 | then ``filter_iterator`` models |
+=======================================================+=================================+
|Readable Iterator, Single Pass Iterator                | Input Iterator                  |
+-------------------------------------------------------+---------------------------------+
|Readable Lvalue Iterator, Forward Traversal Iterator   | Forward Iterator                |
+-------------------------------------------------------+---------------------------------+
|Writable Lvalue Iterator, Forward Traversal Iterator   | Mutable Forward Iterator        |
+-------------------------------------------------------+---------------------------------+
|Writable Lvalue Iterator, Bidirectional Iterator       | Mutable Bidirectional Iterator  |
+-------------------------------------------------------+---------------------------------+


``filter_iterator<P1, X>`` is interoperable with ``filter_iterator<P2, Y>`` 
if and only if ``X`` is interoperable with ``Y``.


``filter_iterator`` operations
..............................

In addition to those operations required by the concepts that
``filter_iterator`` models, ``filter_iterator`` provides the following
operations.


``filter_iterator();``

:Requires: ``Predicate`` and ``Iterator`` must be Default Constructible.
:Effects: Constructs a ``filter_iterator`` whose``m_pred``,  ``m_iter``, and ``m_end`` 
  members are a default constructed.


``filter_iterator(Predicate f, Iterator x, Iterator end = Iterator());``

:Effects: Constructs a ``filter_iterator`` where ``m_iter`` is either
    the first position in the range ``[x,end)`` such that ``f(*m_iter) == true`` 
    or else``m_iter == end``. The member ``m_pred`` is constructed from
    ``f`` and ``m_end`` from ``end``.



``filter_iterator(Iterator x, Iterator end = Iterator());``

:Requires: ``Predicate`` must be Default Constructible and
  ``Predicate`` is a class type (not a function pointer).
:Effects: Constructs a ``filter_iterator`` where ``m_iter`` is either
    the first position in the range ``[x,end)`` such that ``m_pred(*m_iter) == true`` 
    or else``m_iter == end``. The member ``m_pred`` is default constructed.


::

    template <class OtherIterator>
    filter_iterator(
        filter_iterator<Predicate, OtherIterator> const& t
        , typename enable_if_convertible<OtherIterator, Iterator>::type* = 0 // exposition
        );``

:Requires: ``OtherIterator`` is implicitly convertible to ``Iterator``.
:Effects: Constructs a filter iterator whose members are copied from ``t``.


``Predicate predicate() const;``

:Returns: ``m_pred``


``Iterator end() const;``

:Returns: ``m_end``


``Iterator const& base() const;``

:Returns: ``m_iterator``



``reference operator*() const;``

:Returns: ``*m_iter``


``filter_iterator& operator++();``

:Effects: Increments ``m_iter`` and then continues to
  increment ``m_iter`` until either ``m_iter == m_end``
  or ``m_pred(*m_iter) == true``.
:Returns: ``*this``  
