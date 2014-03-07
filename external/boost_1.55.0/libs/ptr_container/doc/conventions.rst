++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Conventions
+++++++++++

There are a few design decisions that will affect how the classes are 
used.  Besides these the classes are much like normal standard containers 
and provides almost the same interface.  The new conventions are: 

.. contents:: :local:

Null pointers are not allowed by default
----------------------------------------

If the user tries to insert the null pointer, the operation will throw a 
``bad_pointer`` exception (see `Example 1 <examples.html>`_).  

Use `nullable <reference.html#class-nullable>`_ to allow null pointers.

Please notice that all preconditions of the form ::

    x != 0;

are not active when the you have instantiated a container
with ``nullable<T>`` as in ::

    boost::ptr_vector< boost::nullable<animal> > vec;
    vec.push_back( 0 ); // ok

All default iterators apply an extra layer of indirection 
--------------------------------------------------------- 

This is done to 
make the containers easier and safer to use.  It promotes a kind of 
pointer-less programming and the user of a class needs not worry about 
pointers except when allocating them (see `Example 2 <examples.html>`_).  Iterators that 
provide access to the naked pointers are also provided since they might be 
useful in rare cases. For example, whenever ``begin()`` returns an iterator, 
``ptr_begin()`` will return an iterator that allows one to iterate over the 
stored pointers.  

All comparison operations are done on the pointed to objects and not at the pointer level
-----------------------------------------------------------------------------------------

For example, in ``ptr_set<T>`` the ordering is by default done by 
``boost::ptr_less<T>`` which compares the indirected pointers. 
Similarly, ``operator==()`` for ``container<Foo>`` compares all objects
with ``operator==(const Foo&, const Foo&)``. 


Stored elements are required to be `Cloneable <reference.html#the-Cloneable-concept>`_ for a subset of the operations
---------------------------------------------------------------------------------------------------------------------

This is because most polymorphic objects cannot be copied directly, but 
they can often be so by a use of a member function (see `Example 4 <examples.html>`_).  Often 
it does not even make sense to clone an object in which case a large 
subset of the operations are still workable.  

Whenever objects are inserted into a container, they are cloned before insertion
--------------------------------------------------------------------------------

This is necessary because all pointer containers take ownerships of stored objects
(see `Example 5 <examples.html>`_).

Whenever pointers are inserted into a container, ownership is transferred to the container
------------------------------------------------------------------------------------------

All containers take ownership of the stored pointers and therefore a 
container needs to have its own copies (see `Example 5 <examples.html>`_).  

Ownership can be transferred from a container on a per pointer basis
--------------------------------------------------------------------

This can of course also be convenient.  Whenever it happens, an 
``SmartContainer::auto_type`` object is used to provide an exception-safe transfer 
(see `Example 6 <examples.html>`_).  

Ownership can be transferred from a container to another container on a per iterator range basis  
------------------------------------------------------------------------------------------------

This makes it possible to exchange data safely between different pointer 
containers without cloning the objects again (see `Example 7 <examples.html>`_).  

A container can be cheaply returned from functions either by making a clone or by giving up ownership of the container
----------------------------------------------------------------------------------------------------------------------

Two special member functions, ``clone()`` and ``release()``, both return an 
``auto_ptr<SmartContainer>`` which can be assigned to another pointer container.  This 
effectively reduces the cost of returning a container to one 
heap-allocation plus a call to ``swap()`` (see `Example 3 <examples.html>`_).

Iterators are invalidated as in the corresponding standard container
--------------------------------------------------------------------

Because the containers in this library wrap standard containers, the
rules for invalidation of iterators are the same as the rules
of the corresponding standard container.

For example, for both ``boost::ptr_vector<T>`` and ``std::vector<U>``
insertion and deletion only invalidates the deleted
element and elements following it; all elements before the inserted/deleted
element remain valid.

.. raw:: html 

        <hr>

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


