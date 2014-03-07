++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png


FAQ
===

.. contents:: :local:
 
Calling ``assign()`` is very costly and I do not really need to store cloned objects; I merely need to overwrite the existing ones; what do I do?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Call ``std::copy( first, last, c.begin() );``.  
 
Which mutating algorithms are safe to use with pointers?
++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Any mutating algorithm that moves elements around by swapping them.  An 
important example is ``std::sort()``; examples of unsafe algorithms are 
``std::unique()`` and ``std::remove()``. 

..  That is why these algorithms are 
    provided as member functions.  

Why does ``ptr_map<T>::insert()/replace()`` take two arguments (the key and the pointer) instead of one ``std::pair``? And why is the key passed by non-const reference?
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This is the only way the function can be implemented in an exception-safe 
manner; since the copy-constructor of the key might throw, and since 
function arguments are not guaranteed to be evaluated from left to right, 
we need to ensure that evaluating the first argument does not throw.  
Passing the key as a reference achieves just that.  

When instantiating a pointer container with a type ``T``, is ``T`` then allowed to be incomplete at that point?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

No. This is a distinct property of ``shared_ptr`` which implies some overhead.

However, one can leave ``T`` incomplete in the header file::

    // foo.hpp
    class Foo { ... };
    new_clone( const Foo& ) { ... }
    delete_clone( const Foo* )     { ... }
    
    // x.hpp
    class Foo; // Foo is incomplete here
    class X { ptr_deque<Foo> container; ... }

    // x.cpp
    #include <x.hpp>
    #include <foo.hpp> // now Foo is not incomplete anymore
    ...
    
    
 
Why do iterator-range inserts give the strong exception-safety guarantee?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Is this not very inefficient?  It is because it is actually affordable to 
do so; the overhead is one heap-allocation which is relatively small 
compared to cloning N objects.  

What is the _`polymorphic class problem`? 
+++++++++++++++++++++++++++++++++++++++++

The problem refers to the relatively troublesome way C++ supports Object 
Oriented programming in connection with containers of pointers to 
polymorphic objects.  In a language without garbage collection, you end up 
using either a container of smart pointers or a container that takes 
ownership of the pointers.  The hard part is to find a safe, fast and 
elegant solution.  

Are the pointer containers faster and do they have a better memory  footprint than a container of smart pointers?  
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

The short answer is yes: they are faster and they do use less memory; in 
fact, they are the only way to obtain the zero-overhead hallmark of C++.  
Smart pointers usually have one word or more of memory overhead per 
pointer because a reference count must be maintained.  And since the 
reference count must be maintained, there is also a runtime-overhead.  If 
your objects are big, then the memory overhead is often negligible, but if 
you have many small objects, it is not.  Further reading can be found in 
these references: `[11] <ptr_container.html#references>`_ and `[12] <ptr_container.html#references>`_.

When the stored pointers cannot be ``0``, how do I allow this "empty" behavior anyway?
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Storing a null-pointer among a list of pointers does not fit well into the Object Oriented paradigm. 
The most elegant design is to use the Null-Object Pattern where one basically makes a concrete
class with dummy implementations of the virtual functions. See `[13] <ptr_container.html#references>`_ for details.

.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


