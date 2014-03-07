++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

================
Usage Guidelines
================

.. contents:: :local: 

Choosing the right container
----------------------------

The recommended usage pattern of the container classes is the same as 
for normal standard containers.  

``ptr_vector``, ``ptr_list`` and ``ptr_deque`` offer the programmer different 
complexity tradeoffs and should be used accordingly.  ``ptr_vector`` is the 
type of sequence that should be used by default.  ``ptr_list`` should be used 
when there are frequent insertions and deletions from the middle of the 
sequence and if the container is fairly large (eg.  more than 100 
elements).  ``ptr_deque`` is the data structure of choice when most insertions 
and deletions take place at the beginning or at the end of the sequence.  
The special container ``ptr_array`` may be used when the size of the container is invariant
and known at compile time.

An associative container supports unique keys if it may contain at most 
one element for each key. Otherwise, it supports equivalent keys.  
``ptr_set`` and ``ptr_map`` support unique keys.  
``ptr_multiset`` and ``ptr_multimap`` 
support equivalent keys.  

Recommended practice for Object-Oriented Programming
----------------------------------------------------

Idiomatic Object-Oriented Programming in C++ looks a bit different from 
the way it is done in other languages. This is partly because C++ 
has both value and reference semantics, and partly because C++ is more flexible
than other languages. Below is a list of recommendations that you are
encouraged to follow:

1. Make base classes abstract and without data
++++++++++++++++++++++++++++++++++++++++++++++

This has the following advantages:

        a. It reduces *coupling* because you do not have to maintain or update state

        ..
                
        b. It helps you to avoid *slicing*
        
        ..
        
        c. It ensures you *override* the right function

You might also want to read the following articles:

- Kevlin Henney's `Six of the best`__

.. __: http://www.two-sdg.demon.co.uk/curbralan/papers/SixOfTheBest.pdf

- Jack Reeves' `Multiple Inheritance Considered Useful`__

.. __: http://www.ddj.com/documents/s=10011/q=1/cuj0602reeves/0602reeves.html

  
2. Make virtual functions private and provide a non-virtual public forwarding function
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

In code::

        class Polymorphic
        {
        private:
            virtual int do_foo() = 0;
            
        public:
            int foo()
            {
                return do_foo();
            }
            ...
        };      
        
This has the following advantages:

        a. It makes sure all calls to the virtual function always goes through one place in your code
        
        ..
        
        b. It enables you to check preconditions and postconditions inside the forwarding function

You might also want to read Herb Sutter's article `Virtuality`__.

.. __: http://www.gotw.ca/publications/mill18.htm

3. Derive your base class from ``boost::noncopyable``
+++++++++++++++++++++++++++++++++++++++++++++++++++++

Having an abstact base class prevents slicing when the base class is involved, but
it does not prevent it for classes further down the hierarchy. This is where
`boost::noncopyable`__ is handy to use::

        class Polymorphic : boost::noncopyable
        {
          ...
        };

.. __ : http://www.boost.org/libs/utility/utility.htm#Class_noncopyable


4. Avoid null-pointers in containers (if possible)
++++++++++++++++++++++++++++++++++++++++++++++++++

By default the pointer containers do not allow you to store null-pointer in them.
As you might know, this behavior can be changed explicitly with the use
of `boost::nullable`__. 

The primary reason to avoid null-pointers 
is that you have to check for null-pointers every time the container is
used. This extra checking is easy to forget, and it is somewhat contradictory to
the spirit of OO where you replace special cases with dynamic dispatch.

.. __: reference.html#class-nullable

Often, however, you need to place some special object in the container because you
do not have enough information to construct a full object. In that case
you might be able to use the Null Object pattern which simply dictates that
you implement virtual functions from the abstract base-class 
as empty functions or with dummy return values. This means that
your OO-code still does not need to worry about null-pointers.

You might want to read

- Kevlin Henney's `Null Object - Something for Nothing`__

.. __: http://www.two-sdg.demon.co.uk/curbralan/papers/europlop/NullObject.pdf

Finally you might end up in a situation where not even the Null Object can help
you. That is when you truly need ``container< nullable<T> >``. 

.. raw:: html 

        <hr>

**Navigate:**

- `home <ptr_container.html>`_
- `reference <reference.html>`_

.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt


