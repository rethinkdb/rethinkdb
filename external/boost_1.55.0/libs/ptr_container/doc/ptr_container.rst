
++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++

.. |Boost| image:: boost.png



:Authors:       Thorsten Ottosen
:Contact:       nesotto@cs.aau.dk or tottosen@dezide.com
:Organizations: `Department of Computer Science`_, Aalborg University, and `Dezide Aps`_
:date:          27th of October 2007
:Copyright:     Thorsten Ottosen 2004-2007. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt

.. _`Department of Computer Science`: http://www.cs.aau.dk
.. _`Dezide Aps`: http://www.dezide.com

========
Overview
========

Boost.Pointer Container provides containers for holding heap-allocated
objects in an exception-safe manner and with minimal overhead.
The aim of the library is in particular to make OO programming
easier in C++ by establishing a standard set of classes, methods
and designs for dealing with OO specific problems

* Motivation_
* Tutorial_
* Reference_
* `Usage guidelines`_
* Examples_
* `Library headers`_
* FAQ_
* `Upgrading from Boost v. 1.33.*`_
* `Upgrading from Boost v. 1.34.*`_
* `Upgrading from Boost v. 1.35.*`_
* `Future Developments`_
* Acknowledgements_
* References_

..
          - `Conventions <conventions.html>`_
          - `The Clonable Concept <reference.html#the-clonable-concept>`_
          - `The Clone Allocator Concept <reference.html#the-clone-allocator-concept>`_
          - `Pointer container adapters <reference.html#pointer-container-adapters>`_
          - `Sequence container classes <reference.html#sequence-containers>`_
        
            - `ptr_vector <ptr_vector.html>`_
            - `ptr_deque <ptr_deque.html>`_
            - `ptr_list <ptr_list.html>`_
            - `ptr_array <ptr_array.html>`_
          - `Associative container classes  <reference.html#associative-containers>`_
        
            - `ptr_set <ptr_set.html>`_
            - `ptr_multiset <ptr_multiset.html>`_
            - `ptr_map <ptr_map.html>`_
            - `ptr_multimap <ptr_multimap.html>`_
          - `Indirected functions <indirect_fun.html>`_
          - `Class nullable <reference.html#class-nullable>`_
          - `Exception classes <reference.html#exception-classes>`_
          


.. _Tutorial: tutorial.html


.. _Reference: reference.html

.. _`Usage guidelines`: guidelines.html

.. _Examples: examples.html

.. _`Library headers`: headers.html

.. _FAQ: faq.html


==========
Motivation
==========

Whenever a programmer wants to have a container of pointers to
heap-allocated objects, there is usually only one exception-safe way:
to make a container of smart pointers like `boost::shared_ptr <../../smart_ptr/shared_ptr.htm>`_
This approach is suboptimal if

1. the stored objects are not shared, but owned exclusively, or

..

2. the overhead implied by smart pointers is inappropriate

This library therefore provides standard-like containers that are for storing
heap-allocated or `cloned <reference.html#the-clonable-concept>`_ objects (or in case of a map, the mapped object must be
a heap-allocated or cloned object). For each of the standard
containers there is a pointer container equivalent that takes ownership of
the objects in an exception safe manner.  In this respect the library is intended
to solve the so-called
`polymorphic class problem <faq.html#what-is-the-polymorphic-class-problem>`_.


The advantages of pointer containers are

1. Exception-safe pointer storage and manipulation.

..

2. Notational convenience compared to the use of containers of pointers.

..

3. Can be used for types that are neither Assignable nor Copy Constructible.

..

4. No memory-overhead as containers of smart pointers can have (see [11]_ and [12]_).

..

5. Usually faster than using containers of smart pointers (see [11]_ and [12]_).

..

6. The interface is slightly changed towards the domain of pointers
   instead of relying on the normal value-based interface. For example,
   now it is possible for ``pop_back()`` to return the removed element.
   
.. 
 
7. Propagates constness such that one cannot modify the objects via a ``const_iterator``.

..

8. Built-in support for deep-copy semantics via the `the Clonable concept`__

.. __: reference.html#the-clonable-concept

The disadvantages are

1. Less flexible than containers of smart pointers like `boost::shared_ptr <../../smart_ptr/shared_ptr.htm>`_

When you do need shared semantics, this library is not what you need.

====================================
 Upgrading from Boost v. ``1.33.*``
====================================

If you upgrade from one of these versions of Boost, then there has been one
major interface change: map iterators now mimic iterators from ``std::map``.
Previously you may have written ::

  for( boost::ptr_map<std::string,T>::iterator i = m.begin(), e = m.end();
       i != e; ++i )
  {
    std::cout << "key:" << i.key();
    std::cout << "value:" << *i;
    i->foo(); // call T::foo()
  }
  
and this now needs to be converted into ::
       
  for( boost::ptr_map<std::string,T>::iterator i = m.begin(), e = m.end();
       i != e; ++i )
  {
    std::cout << "key:" << i->first;
    std::cout << "value:" << *i->second;
    i->second->foo(); // call T::foo()
  }

Apart from the above change, the library now also introduces

- ``std::auto_ptr<T>`` overloads::

        std::auto_ptr<T> p( new T );
        container.push_back( p );

- Derived-to-Base conversion in ``transfer()``::

        boost::ptr_vector<Base>  vec;
        boost::ptr_list<Derived> list;
        ...
        vec.transfer( vec.begin(), list ); // now ok

Also note that `Boost.Assign <../../assign/index.html>`_ introduces better support
for pointer containers. 

====================================
 Upgrading from Boost v. ``1.34.*``
====================================

Serialization has now been made optional thanks to Sebastian Ramacher.
You simply include ``<boost/ptr_container/serialize.hpp>`` or perhaps
just one of the more specialized headers.

All containers are now copy-constructible and assignable. So you can e.g. now
do:: 

    boost::ptr_vector<Derived> derived = ...;
    boost::ptr_vector<Base>    base( derived );
    base = derived;
    
As the example shows, derived-to-base class conversions are also allowed.
 
A few general functions have been added::

    VoidPtrContainer&       base();
    const VoidPtrContainer& base() const;

These allow direct access to the wrapped container which is
sometimes needed when you want to provide extra functionality.
    
A few new functions have been added to sequences::

    void resize( size_type size );
    void resize( size_type size, T* to_clone );

``ptr_vector<T>`` has a few new helper functions to integrate better with C-arrays::

    void transfer( iterator before, T** from, size_type size, bool delete_from = true );
    T**  c_array();

Finally, you can now also "copy" and "assign" an ``auto_type`` ptr by calling ``move()``::

    boost::ptr_vector<T>::auto_type move_ptr = ...;
    return boost::ptr_container::move( move_ptr );

====================================
 Upgrading from Boost v. ``1.35.*``
====================================

The library has been fairly stable, but a few new containers have been supported:

-  ``boost::ptr_unordered_set<T>`` in ``<boost/ptr_container/ptr_unordered_set.hpp>``

-  ``boost::ptr_unordered_map<Key,T>`` in ``<boost/ptr_container/ptr_unordered_map.hpp>``

-  ``boost::ptr_circular_buffer<T>`` in ``<boost/ptr_container/ptr_circular_buffer.hpp>``

There are no docs for these classes yet, but they are almost identical to
``boost::ptr_set<T>``, ``boost::ptr_map<Key,T>`` and ``boost::ptr_array<T,N>``, respectively.
The underlying containers stem from the two boost libraries

- `Boost.Unordered <../../unordered/index.html>`_ 

- `Boost.Circular Buffer <../../circular_buffer/index.html>`_

Furthermore, `insert iterators <ptr_inserter.html>`_ have been added. 

=====================
 Future Developments
=====================

There are indications that the ``void*`` implementation has a slight
performance overhead compared to a ``T*`` based implementation. Furthermore, a 
``T*`` based implementation is so much easier to use type-safely 
with algorithms. Therefore I anticipate to move to a ``T*`` based implementation.

Furthermore, the clone allocator might be allowed to have state. 
This design requires some thought, so if you have good ideas and use-cases'
for this, please don't hesitate to contact me.

Also, support for Boost.Interprocess is on the todo list.

There has been a few request for ``boost::ptr_multi_index_container<T,...>``. 
I investigated how difficult it would be, and it did turn out to be difficult, albeit
not impossible. But I don't have the resources to implement this beast 
for years to come, so if someone really needs this container, I suggest that they
talk with me in private about how it can be done.

================
Acknowledgements
================

The following people have been very helpful:

- Bjørn D. Rasmussen for unintentionally motivating me to start this library
- Pavel Vozenilek for asking me to make the adapters
- David Abrahams for the ``indirect_fun`` design
- Pavol Droba for being review manager
- Ross Boylan for trying out a prototype for real
- Felipe Magno de Almeida for giving fedback based on using the
  library in production code even before the library was part of boost
- Jonathan Turkanis for supplying his ``move_ptr`` framework
  which is used internally
- Stefan Slapeta and Howard Hinnant for Metrowerks support
- Russell Hind for help with Borland compatibility
- Jonathan Wakely for his great help with GCC compatibility and bug fixes
- Pavel Chikulaev for comments and bug-fixes
- Andreas Hommel for fixing the nasty Metrowerks bug
- Charles Brockman for his many comments on the documentation
- Sebastian Ramacher for implementing the optional serialization support

==========
References
==========

.. [1] Matt Austern: `"The Standard Librarian: Containers of Pointers"`__ , C/C++ Users Journal Experts Forum.

__ http://www.cuj.com/documents/s=7990/cujcexp1910austern/

.. [2] Bjarne Stroustrup, "The C++ Programming Language", `Appendix E: "Standard-Library Exception Safety"`__

__ http://www.research.att.com/~bs/3rd_safe.pdf

.. [3] Herb Sutter, "Exceptional C++".
.. [4] Herb Sutter, "More Exceptional C++".
.. [5] Kevlin Henney: `"From Mechanism to Method: The Safe Stacking of Cats"`__ , C++ Experts Forum, February 2002.

__ http://www.cuj.com/documents/s=7986/cujcexp2002henney/henney.htm

.. [6] Some of the few earlier attempts of pointer containers I have seen are the rather interesting NTL_ and the 
       pointainer_. 
       As of this writing both libraries are not exceptions-safe and can leak.

.. [7] INTERNATIONAL STANDARD, Programming languages --- C++, ISO/IEC 14882, 1998. See section 23 in particular.
.. [8] C++ Standard Library Closed Issues List (Revision 27), 
       Item 218, `Algorithms do not use binary predicate objects for default comparisons`__.

__ http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/lwg-closed.html#218       
 
.. [9] C++ Standard Library Active Issues List (Revision 27), 
       Item 226, `User supplied specializations or overloads of namespace std function templates`__. 

__ http://gcc.gnu.org/onlinedocs/libstdc++/ext/lwg-active.html#226

.. [10] Harald Nowak, "A remove_if for vector", C/C++ Users Journal, July 2001.
.. [11] Boost smart pointer timings__

__ http://www.boost.org/libs/smart_ptr/smarttests.htm
 
.. [12] NTL_: Array vs std::vector and boost::shared_ptr 
.. [13] Kevlin Henney, `Null Object`__, 2002.

__ http://www.two-sdg.demon.co.uk/curbralan/papers/europlop/NullObject.pdf

.. _NTL: http://www.ntllib.org/asp.html
.. _pointainer: http://ootips.org/yonat/4dev/pointainer.h 


.. raw:: html 

        <hr>

:Copyright: Thorsten Ottosen 2004-2006. 

