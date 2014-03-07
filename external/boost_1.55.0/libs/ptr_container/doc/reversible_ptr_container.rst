++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``reversible_ptr_container``
------------------------------------

This class is not a real class that can be found in the library. 
Its purpose is to present the general interface of all the pointer containers.

**Hierarchy:**

- ``reversible_ptr_container``

  - `ptr_sequence_adapter <ptr_sequence_adapter.html>`_

    - `ptr_vector <ptr_vector.html>`_
    - `ptr_list <ptr_list.html>`_ 
    - `ptr_deque <ptr_deque.html>`_
    - `ptr_array <ptr_array.html>`_
    
  - `associative_ptr_container <associative_ptr_container.html>`_ 
  
    - `ptr_set_adapter <ptr_set_adapter.html>`_
    - `ptr_multiset_adapter <ptr_multiset_adapter.html>`_
    - `ptr_map_adapter <ptr_map_adapter.html>`_
    - `ptr_multi_map_adapter <ptr_multimap_adapter.html>`_

      - `ptr_set <ptr_set.html>`_
      - `ptr_multi_set <ptr_multiset.html>`_ 
      - `ptr_map <ptr_map.html>`_
      - `ptr_multimap <ptr_multimap.html>`_

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

**synopsis:**

.. parsed-literal:: 
            
        namespace boost
        {      
            template
            < 
                class T, 
                class CloneAllocator,
                class VoidPtrContainer
            >
            class reversible_ptr_container 
            {
            public: // `typedefs`_
                typedef  T*                                           value_type;
                typedef  T&                                           reference;
                typedef  const T&                                     const_reference;
                typedef  *implementation defined*                       iterator;
                typedef  *implementation defined*                       const_iterator;
                typedef  typename VoidPtrContainer::differnce_type    difference_type; 
                typedef  typename VoidPtrContainer::size_type         size_type;
                typedef  typename VoidPtrContainer::allocator_type    allocator_type;
                typedef  *implementation defined*                       reverse_iterator;
                typedef  *implementation defined*                       const_reverse_iterator;
                typedef  *implementation defined*                       auto_type;
            
            public: // `construct/copy/destroy`_
                reversible_ptr_container();
                explicit reversible_ptr_container( const reversible_ptr_container& r );                
                template< class Derived >
                explicit reversible_ptr_container( const reversible_ptr_container<Derived>& r );
                explicit reversible_ptr_container( std::auto_ptr<reversible_ptr_container> r );
                template< class InputIterator >
                reversible_ptr_container( InputIterator first, InputIterator last );
                
                ~reversible_ptr_container();

                reversible_ptr_container&  operator=( const reversible_ptr_container& r );
                template<class Derived>
                reversible_ptr_container&  operator=( const reversible_ptr_container<Derived>& r );
                reversible_ptr_container&  operator=( std::auto_ptr<reversible_ptr_container> r );
                allocator_type             get_allocator() const;                                      
            
            public: // `iterators`_
                iterator                begin();
                const_iterator          begin() const;
                iterator                end();
                const_iterator          end() const;
                reverse_iterator        rbegin();
                const_reverse_iterator  rbegin() const;
                reverse_iterator        rend();
                const_reverse_iterator  rend() const;
                
            public: // `capacity`_
                size_type  size() const;
                size_type  max_size() const;
                bool       empty() const;       
            
            public: // `modifiers`_
                void                     swap( reversible_ptr_container& r );
                void                     clear():
                VoidPtrContainer&        base();
                const VoidPtrContainer&  base() const;
            
            public: // `pointer container requirements`_
                auto_type                                replace( iterator position, T* x );
                template< class U >
                auto_type                                replace( iterator position, std::auto_ptr<U> x );    
                std::auto_ptr<reversible_ptr_container>  clone() const;    
                std::auto_ptr<reversible_ptr_container>  release();
                auto_type                                release( iterator position );
                            
            }; //  class 'reversible_ptr_container'

            // `comparison`_            
            template < class T, class CA, class VPC >
            bool operator==( const reversible_ptr_container<T,CA,VPC>& x,
                             const reversible_ptr_container<T,CA,VPC>& y);
            
            template < class T, class CA, class VPC  >
            bool operator<( const reversible_ptr_container<T,CA,VPC>& x,
                            const reversible_ptr_container<T,CA,VPC>& y);
            
            template < class T, class CA, class VPC  >
            bool operator!=( const reversible_ptr_container<T,CA,VPC>& x,
                             const reversible_ptr_container<T,CA,VPC>& y);
            
            template < class T,  class CA, class VPC >
            bool operator>( const reversible_ptr_container<T,CA,VPC>& x,
                            const reversible_ptr_container<T,CA,VPC>& y);
            
            template < class T,  class CA, class VPC >
            bool operator>=( const reversible_ptr_container<T,CA,VPC>& x,
                             const reversible_ptr_container<T,CA,VPC>& y);
            
            template < class T,  class CA, class VPC >
            bool operator<=( const reversible_ptr_container<T,CA,VPC>& x,
                             const reversible_ptr_container<T,CA,VPC>& y);

            template< class T,  class CA, class VPC  >
            void swap( reversible_ptr_container<T,CA,VPC>& x,
                       reversible_ptr_container<T,CA,VPC>& y );

            // cloneability_
            template< class T,  class CA, class VPC >
            reversible_ptr_container<T,CA,VPC>*
            new_clone( const reversible_ptr_container<T,CA,VPC>& r );

            // `null predicate`_
            template< class Iterator >
            bool is_null( Iterator i );
            
            // `serialization`_
            template<class Archive, class T, class CA, class VPC>
            void serialize( Archive& ar, reversible_ptr_container<T,CÁ,VPC>& c, const unsigned int version );


        } // namespace 'boost'



Semantics
---------

.. _`typedefs`:

Semantics: typedefs
^^^^^^^^^^^^^^^^^^^

Notice how these two types differ:


- ``typedef T* value_type;``

    - notice this has pointer type

- ``typedef T& reference;``

    - notice this is not a pointer type

This is done to be able to add pointers directly
to the container, but to hide the pointers externally.

..
        - ``typedef *implementation defined* object_type;``
        - this is ``T`` for sequences and sets
        - this is ``std::pair<const Key, void*>`` for maps

Also notice that

- ``typedef ... iterator``

allows one to iterate over ``T&`` objects, not ``T*``.
Note that::

    iterator i = ...;
    i.base();

returns an iterator that allows one to iterate over ``void*``
elements (*this is very rarely needed and you should not use the
functionality unless you know what you are doing*).

- ``typedef ... auto_type``

This declaration hides a pointer pointer type. You can rely on the following
operations 

.. parsed-literal:: 

    T* operator->() const;
    T& operator*() const;
    T* release();
    ~auto_type();
    operator *implementation-defined bool*\ ();

The destructor will delete the stored object *using the clone allocator of the container*
(this explains why we cannot use ``std::auto_ptr<T>``). It might help to
think it is just an ``std::auto_ptr<T>``. You can also return
the pointer from a function or assign it to another pointer via the ``move()``
function

.. parsed-literal::

    auto_type ptr   = ...;
    auto_type other = boost::ptr_container::move( ptr );
    return boost::ptr_container::move( other );

.. _construct/copy/destroy:

Semantics: construct/copy/destroy
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``reversible_ptr_container();``

    - Effects: Constructs an empty container

    - Postconditions: ``size() == 0``

..
        - ``reversible_ptr_container( size_type n, const T& x );``

        - Effects: Constructs a container with ``n`` clones of ``x``

        - Postconditions: ``size() == n``
    
- ``explicit reversible_ptr_container( const reversible_ptr_container& r );``

    - Effects: Constructs a container by cloning all elements of ``r``
    
- ``template< class Derived > explicit reversible_ptr_container( const reversible_ptr_container<Derived>& r );``

    - Effects: Constructs a container by cloning all elements of ``r``
    
    - Requirements: ``Derived`` is derived from ``T`` 

- ``explicit reversible_ptr_container( std::auto_ptr< reversible_ptr_container > r );``

    - Effects: Constructs a container by taking ownership of the supplied pointers


- ``template< class InputIterator >``
  ``reversible_ptr_container( InputIterator first, InputIterator last );``

    - Requirements: ``(first,last]`` is a valid range

    - Effects: Constructs a container with a cloned range of ``(first,last]``

    - Postconditions: ``size() == std::distance( first, last )``

- ``~reversible_ptr_container();``

    - Effects: Deletes the stored objects via the clone allocator

    - Throws: Nothing

- ``reversible_ptr_container& operator=( const reversible_ptr_container& r );``

    - Effects: Assigns a clone of ``r``
    
    - Exception safety: strong guarantee
    
- ``template<class Derived> reversible_ptr_container& operator=( const reversible_ptr_container<Derived>& r );``

    - Effects: Assigns a clone of ``r``
    
    - Requirements: ``Derived`` is derived from ``T`` 
    
    - Exception safety: Strong guarantee

- ``reversible_ptr_container& operator=( std::auto_ptr<reversible_ptr_container> r );``

    - Effects: Deletes the stored objects and then takes ownership of the supplied pointers

    - Throws: Nothing

- ``allocator_type get_allocator() const;``

    - Effects: Returns a copy of the allocator of the container object


.. _iterators:

Semantics: iterators
^^^^^^^^^^^^^^^^^^^^

**See also:** `iterator invalidation <conventions.html#iterators-are-invalidated-as-in-the-corresponding-standard-container>`_

- ``iterator begin();``
- ``const_iterator begin() const;``

    - Effects: Returns a mutable/non-mutable iterator with ``value_type T``

    - Throws: Nothing

- ``iterator end();``
- ``const_iterator end() const;``

    - Effects: Returns a mutable/non-mutable iterator with ``value_type T``

    - Throws: Nothing

- ``reverse_iterator rbegin();``

- ``const_reverse_iterator rbegin() const;``

    - Effects: Returns a mutable/non-mutable reverse iterator with ``value_type T``

    - Throws: Nothing

- ``reverse_iterator rend();``

- ``const_reverse_iterator rend() const;``

    - Effects: Returns a mutable/non-mutable reverse iterator with ``value_type T``

    - Throws: Nothing

.. _capacity: 

Semantics: capacity
^^^^^^^^^^^^^^^^^^^

- ``size_type size() const;``

    - Effects: Returns the number of stored elements

    - Throws: Nothing

- ``size_type max_size() const;``

    - Effects: Returns the maximum number of stored elements

    - Throws: Nothing

- ``bool empty() const;``

    - Effects: Returns whether the container is empty or not

    - Throws: Nothing


.. _modifiers:

Semantics: modifiers
^^^^^^^^^^^^^^^^^^^^

- ``void swap( reversible_ptr_container& r );``

    - Effects: Swaps the content of the two containers

    - Throws: Nothing

- ``void clear();``

    - Effects: Destroys all object of the container 

    - Postconditions: ``empty() == true``

    - Throws: Nothing

- ``VoidPtrContainer& base();``

- ``const VoidPtrContainer& base() const;``

    - Returns: a reference to the wrapped container

.. _`pointer container requirements`:

Semantics: pointer container requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``auto_type replace( iterator position, T* x );``

    - Requirements: ``not empty() and x != 0``

    - Effects: returns the object pointed to by ``position`` and replaces it with ``x``.

    - Throws: ``bad_ptr_container_operation`` if the container is empty and ``bad_pointer`` if ``x == 0``.

    - Exception safety: Strong guarantee
    
- ``template< class U > auto_type replace( iterator position, std::auto_ptr<U> x );``

    - Effects: ``return replace( position, x.release() );``     

- ``std::auto_ptr< reversible_ptr_container > clone() const;``

    - Effects: Returns a deep copy of the container

    - Throws: ``std::bad_alloc`` if there is not enough memory to make a clone of the container

    - Complexity: Linear

- ``std::auto_ptr< reversible_ptr_container > release();``

    - Effects: Releases ownership of the container. This is a useful way of returning a container from a function.

    - Postconditions: ``empty() == true``

    - Throws: ``std::bad_alloc`` if the return value cannot be allocated

    - Exception safety: Strong guarantee

- ``auto_type release( iterator position );``

    - Requirements: ``not empty();``

    - Effects: Releases ownership of the pointer referred to by position

    - Postconditions: ``size()`` is one less 

    - Throws: ``bad_ptr_container_operation`` if the container is empty

    - Exception safety: Strong guarantee


.. _comparison:

Semantics: comparison
^^^^^^^^^^^^^^^^^^^^^

These functions compare the underlying range of objects. 
So ::

        operation( const ptr_container& l, const ptr_container& r );

has the effect one would expect of normal standard containers. Hence
objects are compared and not the pointers to objects.

.. _`cloneability`:

Semantics: cloneability
^^^^^^^^^^^^^^^^^^^^^^^

-  ``template< class T, class CloneAllocator >
   reversible_ptr_container<T,CA,VPC>* 
   new_clone( const reversible_ptr_container<T,CA,VPC>& r );``


    - Effects: ``return r.clone().release();``

    - Remarks: This function is only defined for concrete `pointer containers`_, but not for 
      `pointer container adapters`_.

.. _`pointer containers`: ptr_container.html#smart-containers
.. _`pointer container adapters`: ptr_container.html#smart-container-adapters

.. _`null predicate`:

Semantics: null predicate
^^^^^^^^^^^^^^^^^^^^^^^^^

- ``template< class Iterator > bool is_null( Iterator i );``

    - Requirements: ``i`` is a valid dereferencable iterator

    - Returns: ``*i.base() == 0;``

.. _`serialization`:

Semantics: serialization
^^^^^^^^^^^^^^^^^^^^^^^^

All containers can be serialized by means of
`Boost.Serialization`__. For an overview, see
`Serialization of Pointer Containers`_.

.. __: ../../serialization/index.html
.. _`Serialization of Pointer Containers`: reference.html#serialization

 ::
 
    template<class Archive, class T, class CA, class VPC>
    void serialize( Archive& ar, reversible_ptr_container<T,CA,VPC>& c, const unsigned int version );
            

- Effects: Saves or loads the container to/from the archive.

- Remarks: This function is called automatically be stream operators in
  Boost.Serialization
    
- Exception safety: Loading gives the basic guarantee


.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2007. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt

