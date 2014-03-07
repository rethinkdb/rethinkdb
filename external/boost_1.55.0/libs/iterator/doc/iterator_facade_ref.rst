.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. Version 1.3 of this ReStructuredText document corresponds to
   n1530_, the paper accepted by the LWG for TR1.

.. Copyright David Abrahams, Jeremy Siek, and Thomas Witt 2003. 


.. parsed-literal::

  template <
      class Derived
    , class Value
    , class CategoryOrTraversal
    , class Reference  = Value&
    , class Difference = ptrdiff_t
  >
  class iterator_facade {
   public:
      typedef remove_const<Value>::type value_type;
      typedef Reference reference;
      typedef Value\* pointer;
      typedef Difference difference_type;
      typedef /* see below__ \*/ iterator_category;

      reference operator\*() const;
      /* see below__ \*/ operator->() const;
      /* see below__ \*/ operator[](difference_type n) const;
      Derived& operator++();
      Derived operator++(int);
      Derived& operator--();
      Derived operator--(int);
      Derived& operator+=(difference_type n);
      Derived& operator-=(difference_type n);
      Derived operator-(difference_type n) const;
   protected:
      typedef iterator_facade iterator_facade\_;
  };

  // Comparison operators
  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type // exposition
  operator ==(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator !=(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator <(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
             iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator <=(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator >(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
             iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator >=(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

  // Iterator difference
  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  /* see below__ \*/
  operator-(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
            iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

  // Iterator addition
  template <class Dr, class V, class TC, class R, class D>
  Derived operator+ (iterator_facade<Dr,V,TC,R,D> const&,
                     typename Derived::difference_type n);

  template <class Dr, class V, class TC, class R, class D>
  Derived operator+ (typename Derived::difference_type n,
                     iterator_facade<Dr,V,TC,R,D> const&);

__ `iterator category`_

__ `operator arrow`_

__ brackets_

__ minus_

.. _`iterator category`:

The ``iterator_category`` member of ``iterator_facade`` is

.. parsed-literal::

  *iterator-category*\ (CategoryOrTraversal, value_type, reference)

where *iterator-category* is defined as follows:

.. include:: facade_iterator_category.rst

The ``enable_if_interoperable`` template used above is for exposition
purposes.  The member operators should only be in an overload set
provided the derived types ``Dr1`` and ``Dr2`` are interoperable, 
meaning that at least one of the types is convertible to the other.  The
``enable_if_interoperable`` approach uses SFINAE to take the operators
out of the overload set when the types are not interoperable.  
The operators should behave *as-if* ``enable_if_interoperable``
were defined to be::

  template <bool, typename> enable_if_interoperable_impl
  {};

  template <typename T> enable_if_interoperable_impl<true,T>
  { typedef T type; };

  template<typename Dr1, typename Dr2, typename T>
  struct enable_if_interoperable
    : enable_if_interoperable_impl<
          is_convertible<Dr1,Dr2>::value || is_convertible<Dr2,Dr1>::value
        , T
      >
  {};


``iterator_facade`` Requirements
--------------------------------

The following table describes the typical valid expressions on
``iterator_facade``\ 's ``Derived`` parameter, depending on the
iterator concept(s) it will model.  The operations in the first
column must be made accessible to member functions of class
``iterator_core_access``.  In addition,
``static_cast<Derived*>(iterator_facade*)`` shall be well-formed.

In the table below, ``F`` is ``iterator_facade<X,V,C,R,D>``, ``a`` is an
object of type ``X``, ``b`` and ``c`` are objects of type ``const X``,
``n`` is an object of ``F::difference_type``, ``y`` is a constant
object of a single pass iterator type interoperable with ``X``, and ``z``
is a constant object of a random access traversal iterator type
interoperable with ``X``.

.. _`core operations`:

.. topic:: ``iterator_facade`` Core Operations

   +--------------------+----------------------+-------------------------+---------------------------+
   |Expression          |Return Type           |Assertion/Note           |Used to implement Iterator |
   |                    |                      |                         |Concept(s)                 |
   +====================+======================+=========================+===========================+
   |``c.dereference()`` |``F::reference``      |                         |Readable Iterator, Writable|
   |                    |                      |                         |Iterator                   |
   +--------------------+----------------------+-------------------------+---------------------------+
   |``c.equal(y)``      |convertible to bool   |true iff ``c`` and ``y`` |Single Pass Iterator       |
   |                    |                      |refer to the same        |                           |
   |                    |                      |position.                |                           |
   +--------------------+----------------------+-------------------------+---------------------------+
   |``a.increment()``   |unused                |                         |Incrementable Iterator     |
   +--------------------+----------------------+-------------------------+---------------------------+
   |``a.decrement()``   |unused                |                         |Bidirectional Traversal    |
   |                    |                      |                         |Iterator                   |
   +--------------------+----------------------+-------------------------+---------------------------+
   |``a.advance(n)``    |unused                |                         |Random Access Traversal    |
   |                    |                      |                         |Iterator                   |
   +--------------------+----------------------+-------------------------+---------------------------+
   |``c.distance_to(z)``|convertible to        |equivalent to            |Random Access Traversal    |
   |                    |``F::difference_type``|``distance(c, X(z))``.   |Iterator                   |
   +--------------------+----------------------+-------------------------+---------------------------+



``iterator_facade`` operations
------------------------------

The operations in this section are described in terms of operations on
the core interface of ``Derived`` which may be inaccessible
(i.e. private).  The implementation should access these operations
through member functions of class ``iterator_core_access``.

``reference operator*() const;``

:Returns: ``static_cast<Derived const*>(this)->dereference()``

``operator->() const;`` (see below__)

__ `operator arrow`_

:Returns: If ``reference`` is a reference type, an object
  of type ``pointer`` equal to::

    &static_cast<Derived const*>(this)->dereference()

  Otherwise returns an object of unspecified type such that, 
  ``(*static_cast<Derived const*>(this))->m`` is equivalent to ``(w = **static_cast<Derived const*>(this),
  w.m)`` for some temporary object ``w`` of type ``value_type``.

.. _brackets:

*unspecified* ``operator[](difference_type n) const;``

:Returns: an object convertible to ``value_type``. For constant
     objects ``v`` of type ``value_type``, and ``n`` of type
     ``difference_type``, ``(*this)[n] = v`` is equivalent to
     ``*(*this + n) = v``, and ``static_cast<value_type
     const&>((*this)[n])`` is equivalent to
     ``static_cast<value_type const&>(*(*this + n))``



``Derived& operator++();``

:Effects: 

  ::

    static_cast<Derived*>(this)->increment();
    return *static_cast<Derived*>(this);

``Derived operator++(int);``

:Effects:

  ::

    Derived tmp(static_cast<Derived const*>(this));
    ++*this;
    return tmp;


``Derived& operator--();``

:Effects:

   ::

      static_cast<Derived*>(this)->decrement();
      return *static_cast<Derived*>(this);


``Derived operator--(int);``

:Effects:

  ::

    Derived tmp(static_cast<Derived const*>(this));
    --*this;
    return tmp;


``Derived& operator+=(difference_type n);``

:Effects:

  ::

      static_cast<Derived*>(this)->advance(n);
      return *static_cast<Derived*>(this);


``Derived& operator-=(difference_type n);``

:Effects:
 
  ::

      static_cast<Derived*>(this)->advance(-n);
      return *static_cast<Derived*>(this);


``Derived operator-(difference_type n) const;``

:Effects:

  ::

    Derived tmp(static_cast<Derived const*>(this));
    return tmp -= n;

::

  template <class Dr, class V, class TC, class R, class D>
  Derived operator+ (iterator_facade<Dr,V,TC,R,D> const&,
                     typename Derived::difference_type n);

  template <class Dr, class V, class TC, class R, class D>
  Derived operator+ (typename Derived::difference_type n,
                     iterator_facade<Dr,V,TC,R,D> const&);

:Effects:

  ::

    Derived tmp(static_cast<Derived const*>(this));
    return tmp += n;


::

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator ==(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

:Returns: 
  if ``is_convertible<Dr2,Dr1>::value``

  then 
    ``((Dr1 const&)lhs).equal((Dr2 const&)rhs)``.

  Otherwise, 
    ``((Dr2 const&)rhs).equal((Dr1 const&)lhs)``.

::

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator !=(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

:Returns: 
  if ``is_convertible<Dr2,Dr1>::value``

  then 
    ``!((Dr1 const&)lhs).equal((Dr2 const&)rhs)``.

  Otherwise, 
    ``!((Dr2 const&)rhs).equal((Dr1 const&)lhs)``.

::

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator <(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
             iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

:Returns: 
  if ``is_convertible<Dr2,Dr1>::value``

  then 
    ``((Dr1 const&)lhs).distance_to((Dr2 const&)rhs) < 0``.

  Otherwise, 
    ``((Dr2 const&)rhs).distance_to((Dr1 const&)lhs) > 0``.

::

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator <=(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

:Returns: 
  if ``is_convertible<Dr2,Dr1>::value``

  then 
    ``((Dr1 const&)lhs).distance_to((Dr2 const&)rhs) <= 0``.

  Otherwise, 
    ``((Dr2 const&)rhs).distance_to((Dr1 const&)lhs) >= 0``.

::

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator >(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
             iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

:Returns: 
  if ``is_convertible<Dr2,Dr1>::value``

  then 
    ``((Dr1 const&)lhs).distance_to((Dr2 const&)rhs) > 0``.

  Otherwise, 
    ``((Dr2 const&)rhs).distance_to((Dr1 const&)lhs) < 0``.


::

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,bool>::type
  operator >=(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
              iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

:Returns: 
  if ``is_convertible<Dr2,Dr1>::value``

  then 
    ``((Dr1 const&)lhs).distance_to((Dr2 const&)rhs) >= 0``.

  Otherwise, 
    ``((Dr2 const&)rhs).distance_to((Dr1 const&)lhs) <= 0``.

.. _minus:

::

  template <class Dr1, class V1, class TC1, class R1, class D1,
            class Dr2, class V2, class TC2, class R2, class D2>
  typename enable_if_interoperable<Dr1,Dr2,difference>::type
  operator -(iterator_facade<Dr1,V1,TC1,R1,D1> const& lhs,
             iterator_facade<Dr2,V2,TC2,R2,D2> const& rhs);

:Return Type: 
  if ``is_convertible<Dr2,Dr1>::value``

   then 
    ``difference`` shall be
    ``iterator_traits<Dr1>::difference_type``.

   Otherwise 
    ``difference`` shall be ``iterator_traits<Dr2>::difference_type``

:Returns: 
  if ``is_convertible<Dr2,Dr1>::value``

  then 
    ``-((Dr1 const&)lhs).distance_to((Dr2 const&)rhs)``.

  Otherwise, 
    ``((Dr2 const&)rhs).distance_to((Dr1 const&)lhs)``.
