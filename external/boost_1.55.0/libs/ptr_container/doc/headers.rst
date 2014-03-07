++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++

.. |Boost| image:: boost.png

===============
Library headers
===============

======================================================= =============================================================
             **Header**                                    **Includes**

``<boost/ptr_container/clone_allocator.hpp>``            classes heap_clone_allocator_  and view_clone_allocator_
                                                         and functions ``new_clone()`` and ``delete_clone()``

``<boost/ptr_container/ptr_deque.hpp>``                  class `ptr_deque <ptr_deque.html>`_ (and ``std::deque``)

``<boost/ptr_container/ptr_list.hpp>``                   class `ptr_list <ptr_list.html>`_  (and ``std::list``)

``<boost/ptr_container/ptr_vector.hpp>``                 class `ptr_vector <ptr_vector.html>`_ (and ``std::vector``)

``<boost/ptr_container/ptr_array.hpp>``                  class `ptr_array <ptr_array.html>`_ (and ``boost::array``)

``<boost/ptr_container/ptr_set.hpp>``                      classes `ptr_set <ptr_set.html>`_ and `ptr_multiset <ptr_multiset.html>`_
                                                           (and ``std::set`` and ``std::multiset``)

``<boost/ptr_container/ptr_map.hpp>``                      classes `ptr_map <ptr_map.html>`_ and `ptr_multimap <ptr_multimap.html>`_
                                                           (and ``std::map`` and ``std::multimap``)

``<boost/ptr_container/ptr_inserter.hpp>``                 functions `ptr_back_inserter <ptr_inserter.html>`_, `ptr_front_inserter <ptr_inserter.html>`_ and `ptr_inserter <ptr_inserter.html>`_ 

``<boost/ptr_container/ptr_container.hpp>``                all classes

``<boost/ptr_container/ptr_sequence_adapter.hpp>``       class `ptr_sequence_adapter <ptr_sequence_adapter.html>`_

``<boost/ptr_container/ptr_set_adapter.hpp>``            classes `ptr_set_adapter <ptr_set_adapter.html>`_ and `ptr_multiset_adapter <ptr_multiset_adapter.html>`_

``<boost/ptr_container/ptr_map_adapter.hpp>``            classes `ptr_map_adapter <ptr_map_adapter.html>`_ and `ptr_multimap_adapter <ptr_multimap_adapter.html>`_

``<boost/ptr_container/exception.hpp>``                  classes `bad_ptr_container_operation`_, `bad_index`_ and `bad_pointer`_
``<boost/ptr_container/indirect_fun.hpp>``               class `indirect_fun`_

``<boost/ptr_container/nullable.hpp>``                   class `nullable`_

``<boost/ptr_container/serialize_ptr_deque.hpp>``                  class `ptr_deque <ptr_deque.html>`_ with serialization support

``<boost/ptr_container/serialize_ptr_list.hpp>``                   class `ptr_list <ptr_list.html>`_  with serialization support

``<boost/ptr_container/serialize_ptr_vector.hpp>``                 class `ptr_vector <ptr_vector.html>`_ with serialization support

``<boost/ptr_container/serialize_ptr_array.hpp>``                  class `ptr_array <ptr_array.html>`_ with serialization support

``<boost/ptr_container/serialize_ptr_set.hpp>``           classes `ptr_set <ptr_set.html>`_ and `ptr_multiset <ptr_multiset.html>`_ with serialization support

``<boost/ptr_container/serialize_ptr_map.hpp>``           classes `ptr_map <ptr_map.html>`_ and `ptr_multimap <ptr_multimap.html>`_ with serialization support

``<boost/ptr_container/serialize_ptr_container.hpp>``     all classes with serialization support

======================================================= =============================================================

.. _`heap_clone_allocator`: reference.html#the-clone-allocator-concept
.. _`view_clone_allocator`: reference.html#the-clone-allocator-concept
.. _`bad_ptr_container_operation`: reference.html#exception-classes
.. _`bad_index`: reference.html#exception-classes
.. _`bad_pointer`: reference.html#exception-classes
.. _`nullable`: reference.html#class-nullable
.. _`indirect_fun`: indirect_fun.html


**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


