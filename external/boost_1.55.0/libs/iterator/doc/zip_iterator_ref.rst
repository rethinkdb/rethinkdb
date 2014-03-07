.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

  template<typename IteratorTuple>
  class zip_iterator
  {  

  public:
    typedef /* see below */ reference;
    typedef reference value_type;
    typedef value_type* pointer;
    typedef /* see below */ difference_type;
    typedef /* see below */ iterator_category;

    zip_iterator();
    zip_iterator(IteratorTuple iterator_tuple);

    template<typename OtherIteratorTuple>
    zip_iterator(
          const zip_iterator<OtherIteratorTuple>& other
        , typename enable_if_convertible<
                OtherIteratorTuple
              , IteratorTuple>::type* = 0     // exposition only
    );

    const IteratorTuple& get_iterator_tuple() const;

  private:
    IteratorTuple m_iterator_tuple;     // exposition only
  };

  template<typename IteratorTuple> 
  zip_iterator<IteratorTuple> 
  make_zip_iterator(IteratorTuple t);


The ``reference`` member of ``zip_iterator`` is the type of the tuple
made of the reference types of the iterator types in the ``IteratorTuple``
argument.

The ``difference_type`` member of ``zip_iterator`` is the ``difference_type``
of the first of the iterator types in the ``IteratorTuple`` argument.

The ``iterator_category`` member of ``zip_iterator`` is convertible to the
minimum of the traversal categories of the iterator types in the ``IteratorTuple``
argument. For example, if the ``zip_iterator`` holds only vector
iterators, then ``iterator_category`` is convertible to 
``boost::random_access_traversal_tag``. If you add a list iterator, then
``iterator_category`` will be convertible to ``boost::bidirectional_traversal_tag``,
but no longer to ``boost::random_access_traversal_tag``.


``zip_iterator`` requirements
...................................

All iterator types in the argument ``IteratorTuple`` shall model Readable Iterator.  


``zip_iterator`` models
.............................

The resulting ``zip_iterator`` models Readable Iterator.

The fact that the ``zip_iterator`` models only Readable Iterator does not 
prevent you from modifying the values that the individual iterators point
to. The tuple returned by the ``zip_iterator``'s ``operator*`` is a tuple 
constructed from the reference types of the individual iterators, not 
their value types. For example, if ``zip_it`` is a ``zip_iterator`` whose
first member iterator is an ``std::vector<double>::iterator``, then the
following line will modify the value which the first member iterator of
``zip_it`` currently points to:

::

    zip_it->get<0>() = 42.0;


Consider the set of standard traversal concepts obtained by taking
the most refined standard traversal concept modeled by each individual
iterator type in the ``IteratorTuple`` argument.The ``zip_iterator`` 
models the least refined standard traversal concept in this set.

``zip_iterator<IteratorTuple1>`` is interoperable with
``zip_iterator<IteratorTuple2>`` if and only if ``IteratorTuple1``
is interoperable with ``IteratorTuple2``.



``zip_iterator`` operations
.................................

In addition to the operations required by the concepts modeled by
``zip_iterator``, ``zip_iterator`` provides the following
operations.


``zip_iterator();``

:Returns: An instance of ``zip_iterator`` with ``m_iterator_tuple``
  default constructed.


``zip_iterator(IteratorTuple iterator_tuple);``

:Returns: An instance of ``zip_iterator`` with ``m_iterator_tuple``
  initialized to ``iterator_tuple``.


::

    template<typename OtherIteratorTuple>
    zip_iterator(
          const zip_iterator<OtherIteratorTuple>& other
        , typename enable_if_convertible<
                OtherIteratorTuple
              , IteratorTuple>::type* = 0     // exposition only
    );

:Returns: An instance of ``zip_iterator`` that is a copy of ``other``.
:Requires: ``OtherIteratorTuple`` is implicitly convertible to ``IteratorTuple``.


``const IteratorTuple& get_iterator_tuple() const;``

:Returns: ``m_iterator_tuple``


``reference operator*() const;``

:Returns: A tuple consisting of the results of dereferencing all iterators in
  ``m_iterator_tuple``.


``zip_iterator& operator++();``

:Effects: Increments each iterator in ``m_iterator_tuple``.
:Returns: ``*this``


``zip_iterator& operator--();``

:Effects: Decrements each iterator in ``m_iterator_tuple``.
:Returns: ``*this``

::

    template<typename IteratorTuple> 
    zip_iterator<IteratorTuple> 
    make_zip_iterator(IteratorTuple t);

:Returns: An instance of ``zip_iterator<IteratorTuple>`` with ``m_iterator_tuple``
  initialized to ``t``.
