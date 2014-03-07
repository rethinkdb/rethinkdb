++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++

.. |Boost| image:: boost.png

Class ``ptr_sequence_adapter``
------------------------------

This section describes all the common operations for all the pointer
sequences:

- `ptr_vector <ptr_vector.html>`_
- `ptr_list <ptr_list.html>`_ 
- `ptr_deque <ptr_deque.html>`_


The ``ptr_sequence_adapter`` is also a concrete class that you can use to create custom pointer
containers from.

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - ``ptr_sequence_adapter``

    - `ptr_vector <ptr_vector.html>`_
    - `ptr_list <ptr_list.html>`_ 
    - `ptr_deque <ptr_deque.html>`_
    - `ptr_array <ptr_array.html>`_

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_


**Synopsis:**

.. parsed-literal::

        namespace boost
        {

            template
            <
                class T,
                class VoidPtrSeq,
                class CloneAllocator = heap_clone_allocator
            >
            class ptr_sequence_adapter
            {
            public: // `construct/copy/destroy`_
                template< class InputIterator >
                assign( InputIterator first, InputIterator last );
                template< class InputRange >
                assign( const InputRange& e );

            public: // `element access`_
                T&        front();
                const T&  front() const;
                T&        back();
                const T&  back() const;

            public: // `modifiers`_
                void      push_back( T* x );
                template< class U >
                void      push_back( std::auto_ptr<U> x );
                auto_type pop_back();
                iterator  insert( iterator position, T* x );
                template< class U >
                iterator  insert( iterator position, std::auto_ptr<U> x );
                template< class InputIterator >
                void      insert( iterator position, InputIterator first, InputIterator last );
                template< class InputRange >
                void      insert( iterator position, const InputRange& r );
                iterator  erase( iterator position );
                iterator  erase( iterator first, iterator last );
                template< class Range >
                iterator  erase( const Range& r );
                void      resize( size_type size );
                void      resize( size_type size, T* to_clone );

            public: // `pointer container requirements`_
                template< class PtrSequence >
                void transfer( iterator before, typename PtrSequence::iterator object,
                               PtrSequence& from );                             
                template< class PtrSequence >
                void transfer( iterator before, typename PtrSequence::iterator first, typename PtrSequence::iterator last,
                               PtrSequence& from );
                void template< class PtrSequence, class Range >
                void transfer( iterator before, const Range& r, PtrSequence& from );
                template< class PtrSequence >
                void transfer( iterator before, PtrSequence& from );

            public: // `algorithms`_

                void sort();
                void sort( iterator first, iterator last );
                template< class Compare >
                void sort( Compare comp );
                template< class Compare >
                void sort( iterator begin, iterator end, Compare comp );

                void unique();
                void unique( iterator first, iterator last );
                template< class Compare >
                void unique( Compare comp );
                template< class Compare >
                void unique( iterator begin, iterator end, Compare comp );

                template< class Pred >
                void erase_if( Pred pred );
                template< class Pred >
                void erase_if( iterator begin, iterator end, Pred pred );

                void merge( ptr_sequence_adapter& r );
                template< class Compare >
                void merge( ptr_sequence_adapter& r, Compare comp );
                void merge( iterator first, iterator last, ptr_sequence_adapter& from );
                template< class Compare >
                void merge( iterator first, iterator last, ptr_sequence_adapter& from, Compare comp );

            public: // `ptr_list interface`_

            public: // `ptr_vector interface`_

            public: // `ptr_deque interface`_

            }; //  class 'ptr_sequence_adapter'

        } // namespace 'boost'

.. _`ptr_list interface`: ptr_list.html
.. _`ptr_vector interface`: ptr_vector.html
.. _`ptr_deque interface`: ptr_deque.html

Semantics
---------

.. _`construct/copy/destroy`:

Semantics: construct/copy/destroy
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``template< class InputIterator >
  void assign( InputIterator first, InputIterator last );``

    - Requirements: ``(first,last]`` is a valid range

    - Effects: ``clear(); insert( first, last );``

    - Postconditions: ``size() == std::distance( first, last );``

    - Exception safety: strong guarantee

- ``template< class InputRange >
  void assign( const InputRange& r );``

    - Effects: ``assign( boost::begin(r), boost::end(r) );``


..
        - ``assign( size_type n, const T& u )``

        - Effects: ``clear(); insert( begin(), n, u );``

        - Postconditions: ``size() == n``

        - Exception safety: Strong guarantee


..
        void resize( size_type sz, const T& x );
        Effects:

        if ( sz > size() )
            insert( end(), sz-size(), x );
            else if ( sz < size() )
            erase( begin()+sz, end() );
            else
            ; //do nothing

        Postconditions: size() == sz

        Exception safety: Strong guarantee


.. _`element access`:

Semantics: element access
^^^^^^^^^^^^^^^^^^^^^^^^^

- ``T& front();``

    - Requirements: ``not empty();``

    - Effects: ``return *begin();``


- ``const T& front() const;``

    - Requirements: ``not empty();``

    - Effects: ``return *begin();``


- ``T& back();``

    - Requirements: ``not empty();``

    - Effects: ``return *--end();``


- ``const T& back() const;``

    - Requirements: ``not empty();``

    - Effects: ``return *--end();``


.. _`modifiers`:

Semantics: modifiers
^^^^^^^^^^^^^^^^^^^^

- ``void push_back( T* x );``

    - Requirements: ``x != 0``

    - Effects: Inserts the pointer into container and takes ownership of it

    - Throws: ``bad_pointer`` if ``x == 0``

    - Exception safety: Strong guarantee

- ``template< class U > void push_back( std::auto_ptr<U> x );``

    - Effects: ``push_back( x.release() );``
    
..
        - ``void push_back( const T& x );``

        - Effects: ``push_back( CloneAllocator::clone( x ) );``

        - Exception safety: Strong guarantee

- ``auto_type pop_back();``

    - Requirements:``not empty()``

    - Effects: Removes the last element in the container

    - Postconditions: ``size()`` is one less

    - Throws: ``bad_ptr_container_operation`` if ``empty() == true``

    - Exception safety: Strong guarantee


- ``iterator insert( iterator position, T* x );``

    - Requirements: ``position`` is a valid iterator from the container and
      ``x != 0``

    - Effects: Inserts ``x`` before ``position`` and returns an iterator pointing to it

    - Throws: ``bad_pointer`` if ``x == 0``

    - Exception safety: Strong guarantee
    
- ``template< class U > iterator insert( iterator position, std::auto_ptr<U> x );``

    - Effects: ``return insert( position, x.release() );``

..
        - ``iterator insert( iterator position, const T& x );``

        - Requirements: ``position`` is a valid iterator from the container

        - Effects: ``return insert( position, CloneAllocator::clone( x ) );``

        - Exception safety: Strong guarantee

        - ``void insert( iterator position, size_type n, const T& x );``

        - Requirements: ``position`` is a valid iterator from the container

        - Effects: Inserts ``n`` clones of ``x`` before position into the container

        - Exception safety: Strong guarantee

- ``template< class InputIterator >
  void insert( iterator position, InputIterator first, InputIterator last );``

    - Requirements: ``position`` is a valid iterator from the container

    - Effects: Inserts a cloned range before ``position``

    - Exception safety: Strong guarantee

- ``template< class InputRange >
  void insert( iterator position, const InputRange& r );``

    - Effects: ``insert( position, boost::begin(r), boost::end(r) );``

- ``iterator erase( iterator position );``

    - Requirements: ``position`` is a valid iterator from the container

    - Effects: Removes the element defined by ``position`` and returns an iterator to the following element

    - Throws: Nothing

- ``iterator erase( iterator first, iterator last );``

    - Requirements: ``[first,last)`` is a valid range

    - Effects: Removes the range of element defined by ``[first,last)`` and returns an iterator to the following element

    - Throws: Nothing

- ``template< class Range >
  void erase( const Range& r );``

    - Effects: ``erase( boost::begin(r), boost::end(r) );``

- ``void resize( size_type size );``

    - Effects: Resizes the container. If elements are erased, it happens from the back. If elements are inserted, it happens at the back.
    
    - Requirements: ``T`` is default constructible
    
    - Postcondition: ``size() == size;``
    
    - Exception safety: Basic guarantee under expansion; nothrow guarantee otherwise
    
- ``void resize( size_type size, T* to_clone );``

    - Effects: Resizes the container. If elements are erased, it happens from the back. If elements are inserted, clones of ``*to_clone`` are inserted at the back.
    
    - Postcondition: ``size() == size;``
    
    - Exception safety: Basic guarantee under expansion; nothrow guarantee otherwise

    - Remarks: ``to_clone == 0`` is valid if the container supports nulls. The container does not take ownership of ``to_clone``.

.. _`pointer container requirements`:

Semantics: pointer container requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use ``transfer()`` to move elements between two containers of the same type. Furthermore,
you can also move elements from a container of type ``T`` to a container of type ``U`` as long as
``T::value_type`` is convertible to ``U::value_type``. An example would be transferring from ``boost::ptr_vector<Derived>``
to ``boost::ptr_deque<Base>``.

(**Remark:** *When moving elements between two different containers, it is your responsibility to make sure the allocators are compatible.* 
*The special latitude of being able to transfer between two different containers is only available for Sequences and not for Associative Containers.*)

..

- ``template< class PtrSequence > void transfer( iterator before, typename PtrSequence::iterator object, PtrSequence& from );``   

    - Effects: Inserts the object defined by ``object`` into the container and remove it from ``from``.
      Insertion takes place before ``before``.

    - Postconditions: If ``from.empty()``, nothing happens. Otherwise
      ``size()`` is one more, ``from.size()`` is one less.

    - Exception safety: Strong guarantee


- ``template< class PtrSequence > void transfer( iterator before, typename PtrSequence::iterator first, typename PtrSequence::iterator last, PtrSequence& from );``

    - Requirements: ``from.size() >= std::distance(first,last)``

    - Effects: Inserts the objects defined by the range ``[first,last)`` into the container and remove it from ``from``.
      Insertion takes place before ``before``.

    - Postconditions: If ``from.empty()``, nothing happens. Otherwise, 
      let ``N == std::distance(first,last);`` then ``size()`` is ``N`` more, ``from.size()`` is ``N`` less.

    - Exception safety: Strong guarantee
    
    - Complexity: Linear or better

- ``void template< class PtrSequence, class Range > void transfer( iterator before, const Range& r, PtrSequence& from );``

    - Effects: ``transfer(before, boost::begin(r), boost::end(r), from);``

- ``template< class PtrSequence> void transfer( iterator before, PtrSequence& from );``

    - Effects: ``transfer(before, from, from);``

.. _`algorithms`:

Semantics: algorithms
^^^^^^^^^^^^^^^^^^^^^

The general requirement for these algorithms is that the container *does not
contain any nulls*.

- ``void sort();``
- ``void sort( iterator first, iterator last );``
- ``template< class Compare > void sort( Compare comp );``
- ``template< class Compare > void sort( iterator begin, iterator end, Compare comp );``

    - Requirements: (versions without ``Compare``) ``bool operator<( const T&, const T& )`` is defined
    - Requirements: (``Compare`` versions) ``Compare`` must take ``const T&`` arguments
    - Effects: sorts the entire container or the specified range
    - Exception safety: nothrow guarantee (the behavior is undefined if the comparison operator throws)
    - Remarks: The versions of ``sort()`` that take two iterators are not available for ``ptr_list``

- ``void unique();``
- ``void unique( iterator first, iterator last );``
- ``template< class Compare > void unique( Compare comp );``
- ``template< class Compare > void unique( iterator begin, iterator end, Compare comp );``

    - Requirements: (versions without ``Compare``) ``bool operator==( const T&, const T& )`` is defined
    - Requirements: (``Compare`` versions) ``Compare`` must take ``const T&`` arguments
    - Effects: removes adjacent and equal objects from the entire container or the specified range
    - Exception safety: nothrow guarantee (the behavior is undefined if the comparison operator throws)
    
- ``template< class Pred > void erase_if( Pred pred );``
- ``template< class Pred > void erase_if( iterator begin, iterator end, Pred pred );``

    - Requirements: ``Pred`` must take an ``const T&`` argument
    - Effects: removes all elements ``t`` for which ``pred(t)`` returns ``true`` from the entire container or the specified range
    - Exception safety: nothrow guarantee (the behavior is undefined if the comparison operator throws)
        
- ``void merge( ptr_sequence_adapter& r );``
- ``template< class Compare > void merge( ptr_sequence_adapter& r, Compare comp );``
- ``void merge( iterator first, iterator last, ptr_sequence_adapter& from );``
- ``template< class Compare > void merge( iterator first, iterator last, ptr_sequence_adapter& from, Compare comp );``

    - Requirements: (``Compare`` versions) ``Compare`` must take ``const T&`` arguments
    - Requirements: both sequences are sorted wrt. the same predicate
    - Effects: transfers the entire container or the specified sequence to the container while
      ensuring the new sequence is also sorted
    - Postconditions: (Container versions) ``r.empty()``  
    - Exception safety: nothrow guarantee (the behavior is undefined if the comparison operator throws)

.. raw:: html 

        <hr>
    
:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt

    
