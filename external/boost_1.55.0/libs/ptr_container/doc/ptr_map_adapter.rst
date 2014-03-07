++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``ptr_map_adapter``
-------------------------

This class is used to build custom pointer containers with
an underlying map-like container. The interface of the class is an extension
of the interface from ``associative_ptr_container``.

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - `associative_ptr_container <associative_ptr_container.html>`_
  
    - `ptr_set_adapter <ptr_set_adapter.html>`_
    - `ptr_multiset_adapter <ptr_multiset_adapter.html>`_
    - ``ptr_map_adapter``
    - `ptr_multi_map_adapter <ptr_multimap_adapter.html>`_

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
                class T,
                class VoidPtrMap, 
                class CloneAllocator = heap_clone_allocator 
            >
            class ptr_map_adapter 
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
                std::pair<iterator,bool>  insert( key_type& k, T* x );                         
		template< class U >
		std::pair<iterator,bool>  insert( const key_type& k, std::auto_ptr<U> x );                         

            public; // `lookup`_
                T&       operator[]( const key_type& key );
                T&       at( const key_type& key );
                const T& at( const key_type& key ) const;
                
            public: // `pointer container requirements`_
                bool      transfer( iterator object, ptr_map_adapter& from );
                size_type transfer( iterator first, iterator last, ptr_map_adapter& from );
                template< class Range >
                size_type transfer( const Range& r, ptr_map_adapter& from );
                size_type transfer( ptr_map_adapter& from );
                    
            }; //  class 'ptr_map_adapter'
        
        } // namespace 'boost'  

            
Semantics
---------

.. _`typedefs`:

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

- ``std::pair<iterator,bool> insert( key_type& k, value_type x );``

    - Requirements: ``x != 0``

    - Effects: Takes ownership of ``x`` and insert it iff there is no equivalent of it already. The bool part of the return value indicates insertion and the iterator points to the element with key ``x``.

    - Throws: bad_pointer if ``x == 0``

    - Exception safety: Strong guarantee


- ``template< class U > std::pair<iterator,bool> insert( const key_type& k, std::auto_ptr<U> x );``                         

   - Equivalent to (but without the ``const_cast``): ``return insert( const_cast<key_type&>(k), x.release() );``

..
        - ``std::pair<iterator,bool> insert( key_type& k, const_reference x );``
    
        - Effects: ``return insert( allocate_clone( x ) );``
    
        - Exception safety: Strong guarantee


.. _`lookup`: 

Semantics: lookup
^^^^^^^^^^^^^^^^^

- ``T& operator[]( const key_type& key );``

    - Effects: returns the object with key ``key`` if it exists; otherwise a new object is allocated and inserted and its reference returned.
    - Exception-safety: Strong guarantee           

- ``T&       at( const key_type& key );``
- ``const T& at( const key_type& jey ) const;``

    - Requirement: the key exists
    - Throws: ``bad_ptr_container_operation`` if the key does not exist                                 

.. _`pointer container requirements`:

Semantics: pointer container requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``bool transfer( iterator object, ptr_map_adapter& from );``

   - Requirements: ``not from.empty()``

   - Effects: Inserts the object defined by ``object`` into the container and remove it from ``from`` 
     iff no equivalent object exists.

   - Returns: whether the object was transfered
   
   - Exception safety: Strong guarantee

- ``size_type transfer( iterator first, iterator last, ptr__set_adapter& from );``

   - Requirements: ``not from.empty()``

   - Effects: Inserts the objects defined by the range ``[first,last)`` into the container and remove it from ``from``.
     An object is only transferred if no equivalent object exists. 

   - Returns: the number of transfered objects
              
   - Exception safety: Basic guarantee

- ``template< class Range > void transfer( const Range& r, ptr_map_adapter& from );``

    - Effects: ``return transfer( boost::begin(r), boost::end(r), from );``
                   
- ``size_type transfer( ptr_set_adapter& from );``

   - Effects: ``return transfer( from.begin(), from.end(), from );``.

.. raw:: html 

        <hr>
 
:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


