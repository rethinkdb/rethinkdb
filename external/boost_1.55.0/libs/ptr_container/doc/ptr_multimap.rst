++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``ptr_multimap``
----------------------

A ``ptr_multimap<Key,T>`` is a pointer container that uses an underlying ``std::multimap<Key,void*>``
to store the pointers.


**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - `associative_ptr_container <associative_ptr_container.html>`_
  
    - `ptr_set_adapter <ptr_set_adapter.html>`_
    - `ptr_multiset_adapter <ptr_multiset_adapter.html>`_
    - `ptr_map_adapter <ptr_map_adapter.html>`_
    - `ptr_multi_map_adapter <ptr_multimap_adapter.html>`_

      - `ptr_set <ptr_set.html>`_
      - `ptr_multi_set <ptr_multiset.html>`_ 
      - `ptr_map <ptr_map.html>`_
      - ``ptr_multimap``

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

.. _reversible_ptr_container: reversible_ptr_container.html 
.. _associative_ptr_container: associative_ptr_container.html
.. _ptr_multimap_adapter: ptr_multimap_adapter.html



**Synopsis:**

.. parsed-literal::

                     
        namespace boost
        {
            template
            < 
                class Key, 
                class T, 
                class Compare        = std::less<Key>,
                class CloneAllocator = heap_clone_allocator,
                class Allocator      = std::allocator< std::pair<const Key,void*> >
            >
            class ptr_multimap : public ptr_multimap_adapter
                                        <
                                            T,
                                            std::multimap<Key,void*,Compare,Allocator>,
                                            CloneAllocator
                                        >
            {
                // see references
                
            }; //  class 'ptr_multimap'
        
        } // namespace 'boost'  


:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


