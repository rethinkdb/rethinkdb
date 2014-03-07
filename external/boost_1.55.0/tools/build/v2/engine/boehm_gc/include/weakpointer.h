#ifndef	_weakpointer_h_
#define	_weakpointer_h_

/****************************************************************************

WeakPointer and CleanUp

    Copyright (c) 1991 by Xerox Corporation.  All rights reserved.

    THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
    OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.

    Permission is hereby granted to copy this code for any purpose,
    provided the above notices are retained on all copies.

    Last modified on Mon Jul 17 18:16:01 PDT 1995 by ellis

****************************************************************************/

/****************************************************************************

WeakPointer

A weak pointer is a pointer to a heap-allocated object that doesn't
prevent the object from being garbage collected. Weak pointers can be
used to track which objects haven't yet been reclaimed by the
collector. A weak pointer is deactivated when the collector discovers
its referent object is unreachable by normal pointers (reachability
and deactivation are defined more precisely below). A deactivated weak
pointer remains deactivated forever.

****************************************************************************/


template< class T > class WeakPointer {
public:

WeakPointer( T* t = 0 )
    /* Constructs a weak pointer for *t. t may be null. It is an error
       if t is non-null and *t is not a collected object. */
    {impl = _WeakPointer_New( t );}

T* Pointer()
    /* wp.Pointer() returns a pointer to the referent object of wp or
       null if wp has been deactivated (because its referent object
       has been discovered unreachable by the collector). */
    {return (T*) _WeakPointer_Pointer( this->impl );}

int operator==( WeakPointer< T > wp2 )
    /* Given weak pointers wp1 and wp2, if wp1 == wp2, then wp1 and
       wp2 refer to the same object. If wp1 != wp2, then either wp1
       and wp2 don't refer to the same object, or if they do, one or
       both of them has been deactivated. (Note: If objects t1 and t2
       are never made reachable by their clean-up functions, then
       WeakPointer<T>(t1) == WeakPointer<T>(t2) if and only t1 == t2.) */
    {return _WeakPointer_Equal( this->impl, wp2.impl );}

int Hash()
    /* Returns a hash code suitable for use by multiplicative- and
       division-based hash tables. If wp1 == wp2, then wp1.Hash() ==
       wp2.Hash(). */
    {return _WeakPointer_Hash( this->impl );}

private:
void* impl;
};

/*****************************************************************************

CleanUp

A garbage-collected object can have an associated clean-up function
that will be invoked some time after the collector discovers the
object is unreachable via normal pointers. Clean-up functions can be
used to release resources such as open-file handles or window handles
when their containing objects become unreachable.  If a C++ object has
a non-empty explicit destructor (i.e. it contains programmer-written
code), the destructor will be automatically registered as the object's
initial clean-up function.

There is no guarantee that the collector will detect every unreachable
object (though it will find almost all of them). Clients should not
rely on clean-up to cause some action to occur immediately -- clean-up
is only a mechanism for improving resource usage.

Every object with a clean-up function also has a clean-up queue. When
the collector finds the object is unreachable, it enqueues it on its
queue. The clean-up function is applied when the object is removed
from the queue. By default, objects are enqueued on the garbage
collector's queue, and the collector removes all objects from its
queue after each collection. If a client supplies another queue for
objects, it is his responsibility to remove objects (and cause their
functions to be called) by polling it periodically.

Clean-up queues allow clean-up functions accessing global data to
synchronize with the main program. Garbage collection can occur at any
time, and clean-ups invoked by the collector might access data in an
inconsistent state. A client can control this by defining an explicit
queue for objects and polling it at safe points.

The following definitions are used by the specification below:

Given a pointer t to a collected object, the base object BO(t) is the
value returned by new when it created the object. (Because of multiple
inheritance, t and BO(t) may not be the same address.)

A weak pointer wp references an object *t if BO(wp.Pointer()) ==
BO(t).

***************************************************************************/

template< class T, class Data > class CleanUp {
public:

static void Set( T* t, void c( Data* d, T* t ), Data* d = 0 )
    /* Sets the clean-up function of object BO(t) to be <c, d>,
       replacing any previously defined clean-up function for BO(t); c
       and d can be null, but t cannot. Sets the clean-up queue for
       BO(t) to be the collector's queue. When t is removed from its
       clean-up queue, its clean-up will be applied by calling c(d,
       t). It is an error if *t is not a collected object. */ 
       {_CleanUp_Set( t, c, d );}

static void Call( T* t )
    /* Sets the new clean-up function for BO(t) to be null and, if the
       old one is non-null, calls it immediately, even if BO(t) is
       still reachable. Deactivates any weak pointers to BO(t). */
       {_CleanUp_Call( t );}

class Queue {public:
    Queue()
        /* Constructs a new queue. */
            {this->head = _CleanUp_Queue_NewHead();}

    void Set( T* t )
        /* q.Set(t) sets the clean-up queue of BO(t) to be q. */
            {_CleanUp_Queue_Set( this->head, t );}

    int Call()
        /* If q is non-empty, q.Call() removes the first object and
           calls its clean-up function; does nothing if q is
           empty. Returns true if there are more objects in the
           queue. */
           {return _CleanUp_Queue_Call( this->head );}

    private:
    void* head;
    };
};

/**********************************************************************

Reachability and Clean-up

An object O is reachable if it can be reached via a non-empty path of
normal pointers from the registers, stacks, global variables, or an
object with a non-null clean-up function (including O itself),
ignoring pointers from an object to itself.

This definition of reachability ensures that if object B is accessible
from object A (and not vice versa) and if both A and B have clean-up
functions, then A will always be cleaned up before B. Note that as
long as an object with a clean-up function is contained in a cycle of
pointers, it will always be reachable and will never be cleaned up or
collected.

When the collector finds an unreachable object with a null clean-up
function, it atomically deactivates all weak pointers referencing the
object and recycles its storage. If object B is accessible from object
A via a path of normal pointers, A will be discovered unreachable no
later than B, and a weak pointer to A will be deactivated no later
than a weak pointer to B.

When the collector finds an unreachable object with a non-null
clean-up function, the collector atomically deactivates all weak
pointers referencing the object, redefines its clean-up function to be
null, and enqueues it on its clean-up queue. The object then becomes
reachable again and remains reachable at least until its clean-up
function executes.

The clean-up function is assured that its argument is the only
accessible pointer to the object. Nothing prevents the function from
redefining the object's clean-up function or making the object
reachable again (for example, by storing the pointer in a global
variable).

If the clean-up function does not make its object reachable again and
does not redefine its clean-up function, then the object will be
collected by a subsequent collection (because the object remains
unreachable and now has a null clean-up function). If the clean-up
function does make its object reachable again and a clean-up function
is subsequently redefined for the object, then the new clean-up
function will be invoked the next time the collector finds the object
unreachable.

Note that a destructor for a collected object cannot safely redefine a
clean-up function for its object, since after the destructor executes,
the object has been destroyed into "raw memory". (In most
implementations, destroying an object mutates its vtbl.)

Finally, note that calling delete t on a collected object first
deactivates any weak pointers to t and then invokes its clean-up
function (destructor).

**********************************************************************/

extern "C" {
    void* _WeakPointer_New( void* t );
    void* _WeakPointer_Pointer( void* wp );
    int _WeakPointer_Equal( void* wp1, void* wp2 );
    int _WeakPointer_Hash( void* wp );
    void _CleanUp_Set( void* t, void (*c)( void* d, void* t ), void* d );
    void _CleanUp_Call( void* t );
    void* _CleanUp_Queue_NewHead ();
    void _CleanUp_Queue_Set( void* h, void* t );
    int _CleanUp_Queue_Call( void* h );
}

#endif /* _weakpointer_h_ */


