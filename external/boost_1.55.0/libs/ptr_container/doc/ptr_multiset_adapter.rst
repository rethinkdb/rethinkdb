++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``ptr_multiset_adapter``
------------------------------

This class is used to build custom pointer containers with
an underlying multiset-like container. The interface of the class is an extension
of the interface from ``associative_ptr_container``.

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - `associative_ptr_container <associative_ptr_container.html>`_
  
    - `ptr_set_adapter <ptr_set_adapter.html>`_
    - ``ptr_multiset_adapter``
    - `ptr_map_adapter <ptr_map_adapter.html>`_
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
                class Key,
                class VoidPtrMultiSet,
                class CloneAllocator = heap_clone_allocator
            >
            class ptr_multiset_adapter 
            {
                
            public: // `modifiers`_         
                iterator  insert( Key* x );    
		template< class Key2 >
		iterator  insert( std::auto_ptr<Key2> x );                     
 
            public: // `pointer container requirements`_
                void      transfer( iterator object, ptr_multiset_adapter& from );
                size_type transfer( iterator first, iterator last, ptr_multiset_adapter& from );
                template< class Range >
                size_type transfer( const Range& r, ptr_multiset_adapter& from );
                void      transfer( ptr_multiset_adapter& from );
 
            }; //  class 'ptr_multiset_adapter'
        
        } // namespace 'boost'  

            
Semantics
---------

.. _`modifiers`:

Semantics: modifiers
^^^^^^^^^^^^^^^^^^^^

- ``iterator insert( key_type* x );``

    - Requirements: ``x != 0``

    - Effects: Takes ownership of ``x``. The returned iterator points to the element with key ``x``.

    - Throws: bad_pointer if ``x == 0``

    - Exception safety: Strong guarantee

    
- ``template< class Key2 > iterator insert( std::auto_ptr<Key2> x );``

    - Effects: ``return insert( x.release() );``   	                      

.. 
        - ``iterator insert( const key_type& x );``
    
        - Effects: ``return insert( allocate_clone( x ) );``
    
        - Exception safety: Strong guarantee

.. _`pointer container requirements`:

Semantics: pointer container requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``void transfer( iterator object, ptr_multiset_adapter& from );``

   - Requirements: ``not from.empty()``

   - Effects: Inserts the object defined by ``object`` into the container and remove it from ``from``. 

   - Postconditions: ``size()`` is one more, ``from.size()`` is one less.

   - Exception safety: Strong guarantee

- ``void transfer( iterator first, iterator last, ptr_multiset_adapter& from );``

   - Requirements: ``not from.empty()``

   - Effects: Inserts the objects defined by the range ``[first,last)`` into the container and remove it from ``from``.

   - Postconditions: Let ``N == std::distance(first,last);`` then ``size()`` is ``N`` more, ``from.size()`` is ``N`` less.
              
   - Exception safety: Basic guarantee

- ``template< class Range > void transfer( const Range& r, ptr_multiset_adapter& from );``

    - Effects: ``transfer( boost::begin(r), boost::end(r), from );``

- ``void transfer( ptr_multiset_adapter& from );``

   - Effects: ``transfer( from.begin(), from.end(), from );``.

   - Postconditions: ``from.empty();``

   - Exception safety: Basic guarantee
 
.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


