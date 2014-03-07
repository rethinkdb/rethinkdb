++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

========
Examples
========

Some examples are given here and in the accompanying test files:

.. contents:: :local: 


.. _`Example 1`:

1. Null pointers cannot be stored in the containers 
+++++++++++++++++++++++++++++++++++++++++++++++++++

::

        my_container.push_back( 0 );            // throws bad_ptr 
        my_container.replace( an_iterator, 0 ); // throws bad_ptr
        my_container.insert( an_iterator, 0 );  // throws bad_ptr       
        std::auto_ptr<T> p( 0 );
        my_container.push_back( p );            // throws bad_ptr                                                          

.. _`Example 2`:

2. Iterators and other operations return indirected values 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

:: 

        ptr_vector<X> pvec; 
        std::vector<X*> vec;
        *vec.begin()  = new X;   // fine, memory leak
        *pvec.begin() = new X;   // compile time error
        ( *vec.begin() )->foo(); // call X::foo(), a bit clumsy
        pvec.begin()->foo();     // no indirection needed
        *vec.front()  = X();     // overwrite first element
        pvec.front()  = X();     // no indirection needed


.. _`Example 3`:

3. Copy-semantics of pointer containers
+++++++++++++++++++++++++++++++++++++++

::

        ptr_vector<T> vec1; 
        ...
        ptr_vector<T> vec2( vec1.clone() ); // deep copy objects of 'vec1' and use them to construct 'vec2', could be very expensive
        vec2 = vec1.release();              // give up ownership of pointers in 'vec1' and pass the ownership to 'vec2', rather cheap
        vec2.release();                     // give up ownership; the objects will be deallocated if not assigned to another container
        vec1 = vec2;                        // deep copy objects of 'vec2' and assign them to 'vec1', could be very expensive 
        ptr_vector<T> vec3( vec1 );         // deep copy objects of 'vec1', could be very expensive


.. _`Example 4`:

4. Making a non-copyable type Cloneable
+++++++++++++++++++++++++++++++++++++++

::
        
         // a class that has no normal copy semantics
        class X : boost::noncopyable { public: X* clone() const; ... };
                                                                           
        // this will be found by the library by argument dependent lookup (ADL)                                                                  
        X* new_clone( const X& x ) 
        { return x.clone(); }
                                                                           
        // we can now use the interface that requires cloneability
        ptr_vector<X> vec1, vec2;
        ...
        vec2 = vec1.clone();                                 // 'clone()' requires cloning <g> 
        vec2.insert( vec2.end(), vec1.begin(), vec1.end() ); // inserting always means inserting clones 


.. _`Example 5`:

5. Objects are cloned before insertion, inserted pointers are owned by the container 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

::

        class X { ... };                     // assume 'X' is Cloneable 
        X x;                                 // and 'X' can be stack-allocated 
        ptr_list<X> list; 
        list.push_back( new_clone( x ) );    // insert a clone
        list.push_back( new X );             // always give the pointer directly to the container to avoid leaks
        list.push_back( &x );                // don't do this!!! 
        std::auto_ptr<X> p( new X );
        list.push_back( p );                 // give up ownership
        BOOST_ASSERT( p.get() == 0 );


.. _`Example 6`:

6. Transferring ownership of a single element 
+++++++++++++++++++++++++++++++++++++++++++++

::

        ptr_deque<T>                    deq; 
        typedef ptr_deque<T>::auto_type auto_type;
        
        // ... fill the container somehow
        
        auto_type ptr  = deq.release_back();             // remove back element from container and give up ownership
        auto_type ptr2 = deq.release( deq.begin() + 2 ); // use an iterator to determine the element to release
        ptr            = deq.release_front();            // supported for 'ptr_list' and 'ptr_deque'
                                        
        deq.push_back( ptr.release() );                  // give ownership back to the container
        

.. _`Example 7`:

7. Transferring ownership of pointers between different pointer containers 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

::


        ptr_list<X> list; ptr_vector<X> vec;
        ...
        //
        // note: no cloning happens in these examples                                
        //
        list.transfer( list.begin(), vec.begin(), vec );           // make the first element of 'vec' the first element of 'list'
        vec.transfer( vec.end(), list.begin(), list.end(), list ); // put all the lists element into the vector                                 
                      
We can also transfer objects from ``ptr_container<Derived>`` to ``ptr_container<Base>`` without any problems.              

.. _`Example 8`:



8. Selected test files 
++++++++++++++++++++++

:incomplete_type_test.cpp_: Shows how to implement the Composite pattern.
:simple_test.cpp_: Shows how the usage of pointer container compares with a 
  container of smart pointers
:view_example.cpp_: Shows how to use a pointer container as a view into other container
:tree_test.cpp_: Shows how to make a tree-structure
:array_test.cpp_: Shows how to make an n-ary tree 

.. _incomplete_type_test.cpp : ../test/incomplete_type_test.cpp
.. _simple_test.cpp : ../test/simple_test.cpp
.. _view_example.cpp : ../test/view_example.cpp
.. _tree_test.cpp : ../test/tree_test.cpp
.. _array_test.cpp : ../test/ptr_array.cpp



9. A large example
++++++++++++++++++

This example shows many of the most common
features at work. The example provide lots of comments.
The source code can also be found `here <../test/tut1.cpp>`_. 

.. raw:: html
        :file: tutorial_example.html

..
                10. Changing the Clone Allocator
                ++++++++++++++++++++++++++++++++

                This example shows how we can change 
                the Clone Allocator to use the pointer containers
                as view into other containers:

                .. raw:: html
                        :file: tut2.html

.. raw:: html 

        <hr>

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


