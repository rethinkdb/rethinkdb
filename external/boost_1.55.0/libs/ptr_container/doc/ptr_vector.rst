++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``ptr_vector``
--------------------

A ``ptr_vector<T>`` is a pointer container that uses an underlying ``std::vector<void*>``
to store the pointers. 

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - `ptr_sequence_adapter <ptr_sequence_adapter.html>`_

    - ``ptr_vector``
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
                class CloneAllocator = heap_clone_allocator,
                class Allocator      = std::allocator<void*>
            >
            class ptr_vector : public ptr_sequence_adapter
                                      <
                                          T,
                                          std::vector<void*,Allocator>,
                                          CloneAllocator
                                      >
            {
            public: // `construction`_
                explicit ptr_vector( size_type to_reserve );
            
            public: // capacity_
                size_type  capacity() const;
                void       reserve( size_type n );
            
            public: // `element access`_
                T&        operator[]( size_type n );
                const T&  operator[]( size_type n ) const;
                T&        at( size_type n );
                const T&  at( size_type n ) const;

            public: // `pointer container requirements`_
               auto_type replace( size_type idx, T* x );  
               template< class U >
               auto_type replace( size_type idx, std::auto_ptr<U> x );  
               bool      is_null( size_type idx ) const;
               
            public: // `C-array support`_
               void transfer( iterator before, T** from, size_type size, bool delete_from = true );
               T**  c_array();

            };
           
        } // namespace 'boost'  


Semantics
---------

.. _`construction`:

Semantics: construction
^^^^^^^^^^^^^^^^^^^^^^^

- ``explicit ptr_vector( size_type to_reserve );``

    - constructs an empty vector with a buffer
      of size least ``to_reserve``

.. _`capacity`:

Semantics: capacity
^^^^^^^^^^^^^^^^^^^

- ``size_type capacity() const;``

    - Effects: Returns the size of the allocated buffer

    - Throws: Nothing

- ``void reserve( size_type n );``

    - Requirements: ``n <= max_size()``
                 
    - Effects: Expands the allocated buffer

    - Postcondition: ``capacity() >= n``

    - Throws: ``std::length_error()`` if ``n > max_size()``


.. _`element access`:

Semantics: element access
^^^^^^^^^^^^^^^^^^^^^^^^^

- ``T& operator[]( size_type n );``
- ``const T& operator[]( size_type n ) const;``

    - Requirements: ``n < size()``

    - Effects: Returns a reference to the ``n``'th element

    - Throws: Nothing

- ``T& at( size_type n );``
- ``const T& at( size_type n ) const;``

    - Requirements: ``n < size()``

    - Effects: Returns a reference to the ``n``'th element

    - Throws: ``bad_index`` if ``n >= size()``


.. _`pointer container requirements`:

Semantics: pointer container requirements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``auto_type replace( size_type idx, T* x );``    

    - Requirements: `` x != 0 and idx < size()``

    - Effects: returns the object indexed by ``idx`` and replaces it with ``x``.

    - Throws: ``bad_index`` if ``idx >= size()`` and ``bad_pointer`` if ``x == 0``.

    - Exception safety: Strong guarantee

- ``template< class U > auto_type replace( size_type idx, std::auto_ptr<U> x );``

    - Effects: ``return replace( idx, x.release() );``  

- ``bool is_null( size_type idx ) const;``

    - Requirements: ``idx < size()``

    - Effects: returns whether the pointer at index ``idx`` is null

    - Exception safety: Nothrow guarantee


.. _`C-array support`:

Semantics: C-array support
^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``void transfer( iterator before, T** from, size_type size, bool delete_from = true );``

    - Requirements:  ``from != 0``
    
    - Effects: Takes ownership of the dynamic array ``from``
    
    - Exception safety: Strong guarantee if ``delete_from == true``; if ``delete_from == false``,
      and an exception is thrown, the container fails to take ownership.                  
    
    - Remarks: Eventually calls ``delete[] from`` if ``delete_from == true``.   
         
- ``T** c_array();``

    - Returns: ``0`` if the container is empty; otherwise a pointer to the first element of the stored array

    - Throws: Nothing
    
.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2007. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


