++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``ptr_multimap_adapter``
------------------------------

This class is used to build custom pointer containers with
an underlying multimap-like container. The interface of the class is an extension
of the interface from ``associative_ptr_container``.

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - `associative_ptr_container <associative_ptr_container.html>`_
  
    - `ptr_set_adapter <ptr_set_adapter.html>`_
    - `ptr_multiset_adapter <ptr_multiset_adapter.html>`_
    - `ptr_map_adapter <ptr_map_adapter.html>`_
    - ``ptr_multi_map_adapter``

      - `ptr_set <ptr_set.html>`_
      - `ptr_multi_set <ptr_multiset.html>`_ 
      - `ptr_map <ptr_map.html>`_
      - `ptr_multimap <ptr_multimap.html>`_

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

**Synopsis:**

.. parsed-literal::

                     
        namespace boost
        {
            template
            < 
                T,
                class VoidPtrMultiMap,
                class CloneAllocator = heap_clone_allocator 
            >
            class ptr_multimap_adapter 
            {
    	    public: // `typedefs`_
		typedef VoidPtrMap::key_type key_type;
		typedef T*                   mapped_type;
		typedef T&                   mapped_reference;
		typedef const T&             const_mapped_reference;
		typedef ...                  value_type;
		typedef ...                  reference;
		typedef ...                  const_reference;
		typedef ...                  pointer;
		typedef ...                  const_pointer;  
                
            public: // `modifiers`_         
                iterator  insert( key_type& k, T* x ); 
		template< class U >
		iterator  insert( const key_type&, std::auto_ptr<U> x );                        

            public: // `pointer container requirements`_
                void      transfer( iterator object, ptr_multimap_adapter& from );
                size_type transfer( iterator first, iterator last, ptr_multimap_adapter& from );
                template< class Range >
                size_type transfer( const Range& r, ptr_multimap_adapter& from );
                void      transfer( ptr_multimap_adapter& from );

            }; //  class 'ptr_multimap_adapter'
        
        } // namespace 'boost'  

            
Semantics
---------

. _`typedefs`:

Semantics: typedefs
^^^^^^^^^^^^^^^^^^^

The following types are implementation defined::

	typedef ... value_type;
	typedef ... reference;
	typedef ... const_reference;
	typedef ... pointer;
	typedef ... const_pointer;  
        
However, the structure of the type mimics ``std::pair`` s.t. one
can use ``first`` and ``second`` members. The reference-types
are not real references and the pointer-types are not real pointers.
However, one may still write ::

    map_type::value_type       a_value      = *m.begin();
    a_value.second->foo();
    map_type::reference        a_reference  = *m.begin();
    a_reference.second->foo();
    map_type::const_reference  a_creference = *const_begin(m);
    map_type::pointer          a_pointer    = &*m.begin();
    a_pointer->second->foo();
    map_type::const_pointer    a_cpointer   = &*const_begin(m);

The difference compared to ``std::map<Key,T*>`` is that constness
is propagated to the pointer (that is, to ``second``) in ``const_itertor``. 	

.. _`modifiers`:

Semantics: modifiers
^^^^^^^^^^^^^^^^^^^^

- ``iterator insert( key_type& k, T* x );``

    - Requirements: ``x != 0``

    - Effects: Takes ownership of ``x`` and returns an iterator pointing to it.

    - Throws: bad_pointer if ``x == 0``

    - Exception safety: Strong guarantee

- ``template< class U > iterator insert( const key_type& k, std::auto_ptr<U> x );``                         

   - Equivalent to (but without the ``const_cast``): ``return insert( const_cast<key_type&>(k), x.release() );``

.. 
        - ``iterator insert( key_type& k, const_reference x );``
    
        - Effects: ``return insert( allocate_clone( x ) );``
    
        - Exception safety: Strong guarantee


.. _`lookup`: 

..
        Semantics: lookup
        ^^^^^^^^^^^^^^^^^
        
        - ``reference        operator[]( const Key& key );``
        - ``const_reference  operator[]( const Key& key ) const;``
        
            - Requirements: the key exists
        
            - Effects: returns the object with key ``key``
        
            - Throws: ``bad_ptr_container_operation`` if the key does not exist                                 

.. _`pointer container requirements`:
        
Semantics: pointer container requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``void transfer( iterator object, ptr_multimap_adapter& from );``

   - Requirements: ``not from.empty()``

   - Effects: Inserts the object defined by ``object`` into the container and remove it from ``from``. 

   - Postconditions: ``size()`` is one more, ``from.size()`` is one less.

   - Exception safety: Strong guarantee

- ``void transfer( iterator first, iterator last, ptr_multimap_adapter& from );``

   - Requirements: ``not from.empty()``

   - Effects: Inserts the objects defined by the range ``[first,last)`` into the container and remove it from ``from``.

   - Postconditions: Let ``N == std::distance(first,last);`` then ``size()`` is ``N`` more, ``from.size()`` is ``N`` less.
              
   - Exception safety: Basic guarantee

- ``template< class Range > void transfer( const Range& r, ptr_multimap_adapter& from );``

    - Effects: ``transfer( boost::begin(r), boost::end(r), from );``

- ``void transfer( ptr_multimap_adapter& from );``

   - Effects: ``transfer( from.begin(), from.end(), from );``.

   - Postconditions: ``from.empty();``

   - Exception safety: Basic guarantee
 
.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


