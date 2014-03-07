++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Class ``ptr_set``
-----------------

A ``ptr_set<T>`` is a pointer container that uses an underlying ``std::set<void*>``
to store the pointers.

**Hierarchy:**

- `reversible_ptr_container <reversible_ptr_container.html>`_

  - `associative_ptr_container <associative_ptr_container.html>`_
  
    - `ptr_set_adapter <ptr_set_adapter.html>`_
    - `ptr_multiset_adapter  <ptr_multiset_adapter.html>`_
    - `ptr_map_adapter <ptr_map_adapter.html>`_
    - `ptr_multi_map_adapter <ptr_multimap_adapter.html>`_

      - ``ptr_set``
      - `ptr_multi_set <ptr_multiset.html>`_ 
      - `ptr_map <ptr_map.html>`_
      - `ptr_multimap <ptr_multimap.html>`_

    
**See also:**

- `void_ptr_indirect_fun <indirect_fun.html>`_

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
                class Compare        = std::less<Key>, 
                class CloneAllocator = heap_clone_allocator, 
                class Allocator      = std::allocator<void*>
            >
            class ptr_set : public  ptr_set_adapter
                                    <
                                        Key,
                                        std::set<void*,
                                        void_ptr_indirect_fun<Compare,Key>,Allocator>,
                                        CloneAllocator
                                    >
            {
                // see references
                
            }; //  class 'ptr_set'
        
        } // namespace 'boost'  

**Remarks:**

- Using ``nullable<T>`` as ``Key`` is meaningless and is not allowed

.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


