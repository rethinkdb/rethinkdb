.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

  template <class Iterator>
  class reverse_iterator
  {
  public:
    typedef iterator_traits<Iterator>::value_type value_type;
    typedef iterator_traits<Iterator>::reference reference;
    typedef iterator_traits<Iterator>::pointer pointer;
    typedef iterator_traits<Iterator>::difference_type difference_type;
    typedef /* see below */ iterator_category;

    reverse_iterator() {}
    explicit reverse_iterator(Iterator x) ;

    template<class OtherIterator>
    reverse_iterator(
        reverse_iterator<OtherIterator> const& r
      , typename enable_if_convertible<OtherIterator, Iterator>::type* = 0 // exposition
    );
    Iterator const& base() const;
    reference operator*() const;
    reverse_iterator& operator++();
    reverse_iterator& operator--();
  private:
    Iterator m_iterator; // exposition
  };


If ``Iterator`` models Random Access Traversal Iterator and Readable
Lvalue Iterator, then ``iterator_category`` is convertible to
``random_access_iterator_tag``. Otherwise, if
``Iterator`` models Bidirectional Traversal Iterator and Readable
Lvalue Iterator, then ``iterator_category`` is convertible to
``bidirectional_iterator_tag``. Otherwise, ``iterator_category`` is
convertible to ``input_iterator_tag``.



``reverse_iterator`` requirements
.................................

``Iterator`` must be a model of Bidirectional Traversal Iterator.  The
type ``iterator_traits<Iterator>::reference`` must be the type of
``*i``, where ``i`` is an object of type ``Iterator``.



``reverse_iterator`` models
...........................

A specialization of ``reverse_iterator`` models the same iterator
traversal and iterator access concepts modeled by its ``Iterator``
argument.  In addition, it may model old iterator concepts
specified in the following table:

+---------------------------------------+-----------------------------------+
| If ``I`` models                       |then ``reverse_iterator<I>`` models|
+=======================================+===================================+
| Readable Lvalue Iterator,             | Bidirectional Iterator            |
| Bidirectional Traversal Iterator      |                                   |
+---------------------------------------+-----------------------------------+
| Writable Lvalue Iterator,             | Mutable Bidirectional Iterator    |
| Bidirectional Traversal Iterator      |                                   |
+---------------------------------------+-----------------------------------+
| Readable Lvalue Iterator,             | Random Access Iterator            |
| Random Access Traversal Iterator      |                                   |
+---------------------------------------+-----------------------------------+
| Writable Lvalue Iterator,             | Mutable Random Access Iterator    |
| Random Access Traversal Iterator      |                                   |
+---------------------------------------+-----------------------------------+


``reverse_iterator<X>`` is interoperable with
``reverse_iterator<Y>`` if and only if ``X`` is interoperable with
``Y``.

``reverse_iterator`` operations
...............................

In addition to the operations required by the concepts modeled by
``reverse_iterator``, ``reverse_iterator`` provides the following
operations.



``reverse_iterator();``

:Requires: ``Iterator`` must be Default Constructible.
:Effects: Constructs an instance of ``reverse_iterator`` with ``m_iterator`` 
  default constructed.

``explicit reverse_iterator(Iterator x);``

:Effects: Constructs an instance of ``reverse_iterator`` with
    ``m_iterator`` copy constructed from ``x``.


::

    template<class OtherIterator>
    reverse_iterator(
        reverse_iterator<OtherIterator> const& r
      , typename enable_if_convertible<OtherIterator, Iterator>::type* = 0 // exposition
    );

:Requires: ``OtherIterator`` is implicitly convertible to ``Iterator``.
:Effects: Constructs instance of ``reverse_iterator`` whose 
    ``m_iterator`` subobject is constructed from ``y.base()``.



``Iterator const& base() const;``

:Returns: ``m_iterator``


``reference operator*() const;``

:Effects: 

::

    Iterator tmp = m_iterator;
    return *--tmp;


``reverse_iterator& operator++();``

:Effects: ``--m_iterator``
:Returns: ``*this``


``reverse_iterator& operator--();``

:Effects: ``++m_iterator``
:Returns: ``*this``
