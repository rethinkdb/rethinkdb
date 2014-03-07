++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``ptr_deque``
--------------------

A ``ptr_deque<T>`` is a pointer container that uses an underlying ``std:deque<void*>``
to store the pointers. 

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - `ptr_sequence_adapter <ptr_sequence_adapter.html>`_

    - `ptr_vector <ptr_vector.html>`_
    - `ptr_list <ptr_list.html>`_ 
    - ``ptr_deque``
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
                class CloneAllocator = heap_clone_allocator
                class Allocator      = std::allocator<void*>
            >
            class ptr_deque : public ptr_sequence_adapter
                                     <
                                        T,
                                        std::deque<void*,Allocator>,
                                        CloneAllocator
                                     >
            {

            public: // `element access`_
                T&        operator[]( size_type n );
                const T&  operator[]( size_type n ) const;
                T&        at( size_type n );
                const T&  at( size_type n ) const;
    
            public: // modifiers_
                void      push_front( T* x );
		template< class U >
		void      push_front( std::auto_ptr<U> x );
                auto_type pop_front();

            public: // `pointer container requirements`_
               auto_type replace( size_type idx, T* x );
	       template< class U >
	       auto_type replace( size_type idx, std::auto_ptr<U> x );    
               bool      is_null( size_type idx ) const;   
    
            };

        } // namespace 'boost'  


.. _`reversible_ptr_container`: reversible_ptr_container.html 

.. _`ptr_sequence_adapter`: ptr_sequence_adapter.html

Semantics
---------

.. _modifiers:
 
Semantics: modifiers
^^^^^^^^^^^^^^^^^^^^

- ``void push_front( T* x );``

    - Requirements: ``x != 0``

    - Effects: Inserts the pointer into container and takes ownership of it
    
    - Throws: ``bad_pointer`` if ``x == 0``

    - Exception safety: Strong guarantee

- ``template< class U > void push_front( std::auto_ptr<U> x );``

    - Effects: ``push_front( x.release() );``

.. 
        - ``void push_front( const T& x );``
    
        - Effects: push_front( allocate_clone( x ) );
    
        - Exception safety: Strong guarantee

- ``auto_type pop_front():``

    - Requirements:``not empty()``
    
    - Effects: Removes the first element in the container

    - Postconditions: ``size()`` is one less

    - Throws: ``bad_ptr_container_operation`` if ``empty() == true``
    
    - Exception safety: Strong guarantee


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

    - Throws: ``bad_index`` if ``n >=size()``


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

.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


