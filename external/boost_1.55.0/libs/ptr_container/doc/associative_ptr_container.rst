++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``associative_ptr_container``
-------------------------------------

This section describes all the common operations for all associative
pointer containers (in addition to ``reversible_ptr_container``).

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - ``associative_ptr_container`` 
  
    - `ptr_set_adapter <ptr_set_adapter.html>`_
    - `ptr_multiset_adapter <ptr_multiset_adapter.html>`_
    - `ptr_map_adapter <ptr_map_adapter.html>`_
    - `ptr_multi_map_adapter <ptr_multimap_adapter.html>`_

      - `ptr_set <ptr_set.html>`_
      - `ptr_multi_set <ptr_multiset.html>`_ 
      - `ptr_map <ptr_map.html>`_
      - `ptr_multimap <ptr_multimap.html>`_

**See also:**

- `iterator_range <http://www.boost.org/libs/range/doc/utility_class.html#iter_range>`_

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

**Synopsis:**

.. parsed-literal::

        namespace boost
        {
            template
            < 
                class Key, 
                class CloneAllocator = heap_clone_allocator 
            >
            class associative_ptr_container 
            {
            public: // typedefs_
                typedef ...   key_type;
                typedef ...   key_compare;
                typedef ...   value_compare;
        
            public: // `observers`_
                key_compare    key_comp() const;
                value_compare  value_comp() const;
        
            public: // `modifiers`_         
                template< typename InputIterator >
                void       insert( InputIterator first, InputIterator last );     
                template< class InputRange >
                void       insert( const InputRange& r );
                void       erase( iterator position ); 
                size_type  erase( const key_type& x );
                template< class Range >
                void       erase( const Range& r );
                void       erase( iterator first, iterator last );

            public: // `algorithms`_
                iterator                        find( const key_type& x );
                const_iterator                  find( const key_type& x ) const;
                size_type                       count( const key_type& x ) const;              
                iterator                        lower_bound( const key_type& x );                     
                const_iterator                  lower_bound( const key_type& x ) const;
                iterator                        upper_bound( const key_type& x );                           
                const_iterator                  upper_bound( const key_type& x ) const;
                iterator_range<iterator>        equal_range( const key_type& x );                 
                iterator_range<const_iterator>  equal_range( const key_type& x ) const;
             
            }; //  class 'associative_ptr_container'
            
        } // namespace 'boost'  

    
Semantics
---------

.. _typedefs:

Semantics: typedefs
^^^^^^^^^^^^^^^^^^^

- ``typedef ... key_type;``

    - if we are dealing with a map, then simply the key type
    - if we are dealing with a set, then the *indirected* key type, that is, 
      given ``ptr_set<T>``, ``key_type*`` will be ``T*``.

- ``typedef ... key_compare;``

    -  comparison object type that determines the order of elements in the container

- ``typedef ... value_compare;``

    - comparison object type that determines the order of elements in the container
    - if we are dealing with a map, then this comparison simply forwards to the ``key_compare`` comparison operation

.. _`observers`:

Semantics: observers
^^^^^^^^^^^^^^^^^^^^

- ``key_compare key_comp() const;``
- ``value_compare value_comp() const;``

    - returns copies of objects used to determine the order of elements

.. _`modifiers`:

Semantics: modifiers
^^^^^^^^^^^^^^^^^^^^

- ``template< typename InputIterator >
  void insert( InputIterator first, InputIterator last );``

    - Requirements: ``[first,last)`` is a valid range

    - Effects: Inserts a cloned range 

    - Exception safety: Basic guarantee

- ``template< class InputRange >
  void insert( const InputRange& r );``

    - Effects: ``insert( boost::begin(r), boost::end(r) );``

- ``void erase( iterator position );``

    - Requirements: ``position`` is a valid iterator from the container

    - Effects: Removes the element defined by ``position``.

    - Throws: Nothing

- ``size_type erase( const key_type& x );``

    - Effects: Removes all the elements in the container with a key equivalent to ``x`` and returns the number of erased elements.

    - Throws: Nothing

- ``void erase( iterator first, iterator last );``

    - Requirements: ``[first,last)`` is a valid range

    - Effects: Removes the range of elements defined by ``[first,last)``.

    - Throws: Nothing

- ``template< class Range > void erase( const Range& r );``

    - Effects: ``erase( boost::begin(r), boost::end(r) );``

.. _`algorithms`:

Semantics: algorithms
^^^^^^^^^^^^^^^^^^^^^

- ``iterator       find( const Key& x );``
- ``const_iterator find( const Key& x ) const;``

    - Effects: Searches for the key and returns ``end()`` on failure.

    - Complexity: Logarithmic

- ``size_type count( const Key& x ) const;``

    - Effects: Counts the elements with a key equivalent to ``x``

    - Complexity: Logarithmic

- ``iterator       lower_bound( const Key& x );``
- ``const_iterator lower_bound( const Key& x ) const;``

    - Effects: Returns an iterator pointing to the first element with a key not less than ``x``

    - Complexity: Logarithmic

- ``iterator       upper_bound( const Key& x );``
- ``const_iterator upper_bound( const Key& x ) const;``

    - Effects: Returns an iterator pointing to the first element with a key greater than ``x``

    - Complexity: Logarithmic

- ``iterator_range<iterator>       equal_range( const Key& x );`` 
- ``iterator_range<const_iterator> equal_range( const Key& x ) const;`` 

    - Effects: ``return boost::make_iterator_range( lower_bound( x ), upper_bound( x ) );``

    - Complexity: Logarithmic

..
        - ``reference       at( const key_type& key );``
        - ``const_reference at( const key_type& key ) const;`` 
    
        - Requirements: the key exists
    
        - Effects: returns the object with key ``key``
    
        - Throws: ``bad_ptr_container_operation`` if the key does not exist                                 
    

.. _`pointer container requirements`:

.. raw:: html 

        <hr>
	
:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt

