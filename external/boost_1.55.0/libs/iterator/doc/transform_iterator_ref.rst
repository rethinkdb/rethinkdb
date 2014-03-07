.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. Version 1.3 of this document was accepted for TR1

::

  template <class UnaryFunction,
            class Iterator, 
            class Reference = use_default, 
            class Value = use_default>
  class transform_iterator
  {
  public:
    typedef /* see below */ value_type;
    typedef /* see below */ reference;
    typedef /* see below */ pointer;
    typedef iterator_traits<Iterator>::difference_type difference_type;
    typedef /* see below */ iterator_category;

    transform_iterator();
    transform_iterator(Iterator const& x, UnaryFunction f);

    template<class F2, class I2, class R2, class V2>
    transform_iterator(
          transform_iterator<F2, I2, R2, V2> const& t
        , typename enable_if_convertible<I2, Iterator>::type* = 0      // exposition only
        , typename enable_if_convertible<F2, UnaryFunction>::type* = 0 // exposition only
    );
    UnaryFunction functor() const;
    Iterator const& base() const;
    reference operator*() const;
    transform_iterator& operator++();
    transform_iterator& operator--();
  private:
    Iterator m_iterator; // exposition only
    UnaryFunction m_f;   // exposition only
  };


If ``Reference`` is ``use_default`` then the ``reference`` member of
``transform_iterator`` is
``result_of<const UnaryFunction(iterator_traits<Iterator>::reference)>::type``.
Otherwise, ``reference`` is ``Reference``.

If ``Value`` is ``use_default`` then the ``value_type`` member is
``remove_cv<remove_reference<reference> >::type``.  Otherwise,
``value_type`` is ``Value``.


If ``Iterator`` models Readable Lvalue Iterator and if ``Iterator``
models Random Access Traversal Iterator, then ``iterator_category`` is
convertible to ``random_access_iterator_tag``. Otherwise, if
``Iterator`` models Bidirectional Traversal Iterator, then
``iterator_category`` is convertible to
``bidirectional_iterator_tag``.  Otherwise ``iterator_category`` is
convertible to ``forward_iterator_tag``. If ``Iterator`` does not
model Readable Lvalue Iterator then ``iterator_category`` is
convertible to ``input_iterator_tag``.


``transform_iterator`` requirements
...................................

The type ``UnaryFunction`` must be Assignable, Copy Constructible, and
the expression ``f(*i)`` must be valid where ``f`` is a const object of
type ``UnaryFunction``, ``i`` is an object of type ``Iterator``, and
where the type of ``f(*i)`` must be
``result_of<const UnaryFunction(iterator_traits<Iterator>::reference)>::type``.

The argument ``Iterator`` shall model Readable Iterator.  


``transform_iterator`` models
.............................

The resulting ``transform_iterator`` models the most refined of the
following that is also modeled by ``Iterator``.

  * Writable Lvalue Iterator if ``transform_iterator::reference`` is a non-const reference. 

  * Readable Lvalue Iterator if ``transform_iterator::reference`` is a const reference.

  * Readable Iterator otherwise. 

The ``transform_iterator`` models the most refined standard traversal
concept that is modeled by the ``Iterator`` argument.

If ``transform_iterator`` is a model of Readable Lvalue Iterator then
it models the following original iterator concepts depending on what
the ``Iterator`` argument models.

+-----------------------------------+---------------------------------------+
| If ``Iterator`` models            | then ``transform_iterator`` models    |
+===================================+=======================================+
| Single Pass Iterator              | Input Iterator                        |
+-----------------------------------+---------------------------------------+
| Forward Traversal Iterator        | Forward Iterator                      |
+-----------------------------------+---------------------------------------+
| Bidirectional Traversal Iterator  | Bidirectional Iterator                |
+-----------------------------------+---------------------------------------+
| Random Access Traversal Iterator  | Random Access Iterator                |
+-----------------------------------+---------------------------------------+

If ``transform_iterator`` models Writable Lvalue Iterator then it is a
mutable iterator (as defined in the old iterator requirements).

``transform_iterator<F1, X, R1, V1>`` is interoperable with
``transform_iterator<F2, Y, R2, V2>`` if and only if ``X`` is
interoperable with ``Y``.



``transform_iterator`` operations
.................................

In addition to the operations required by the concepts modeled by
``transform_iterator``, ``transform_iterator`` provides the following
operations.


``transform_iterator();``

:Returns: An instance of ``transform_iterator`` with ``m_f``
  and ``m_iterator`` default constructed.


``transform_iterator(Iterator const& x, UnaryFunction f);``

:Returns: An instance of ``transform_iterator`` with ``m_f``
  initialized to ``f`` and ``m_iterator`` initialized to ``x``.


::

    template<class F2, class I2, class R2, class V2>
    transform_iterator(
          transform_iterator<F2, I2, R2, V2> const& t
        , typename enable_if_convertible<I2, Iterator>::type* = 0      // exposition only
        , typename enable_if_convertible<F2, UnaryFunction>::type* = 0 // exposition only
    );

:Returns: An instance of ``transform_iterator`` with ``m_f``
  initialized to ``t.functor()`` and ``m_iterator`` initialized to
  ``t.base()``.
:Requires: ``OtherIterator`` is implicitly convertible to ``Iterator``.


``UnaryFunction functor() const;``

:Returns: ``m_f``


``Iterator const& base() const;``

:Returns: ``m_iterator``


``reference operator*() const;``

:Returns: ``m_f(*m_iterator)``


``transform_iterator& operator++();``

:Effects: ``++m_iterator``
:Returns: ``*this``


``transform_iterator& operator--();``

:Effects: ``--m_iterator``
:Returns: ``*this``

