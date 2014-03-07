:Author:
    `Dean Michael Berris <mailto:me@deanberris.com>`_

:License:
    Distributed under the Boost Software License, Version 1.0
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

:Copyright:
    Copyright 2012 Google, Inc.

Function Input Iterator
=======================

The Function Input Iterator allows for creating iterators that encapsulate
a nullary function object and a state object which tracks the number of times
the iterator has been incremented. A Function Input Iterator models the
`InputIterator`_ concept and is useful for creating bounded input iterators.

.. _InputIterator: http://www.sgi.com/tech/stl/InputIterator.html

The Function Input Iterator takes a function that models the Generator_ concept
(which is basically a nullary or 0-arity function object). The first dereference
of the iterator at a given position invokes the generator function and stores
and returns the result; subsequent dereferences at the same position simply
return the same stored result. Incrementing the iterator places it at a new
position, hence a subsequent dereference will generate a new value via another
invokation of the generator function. This ensures the generator function is
invoked precisely when the iterator is requested to return a (new) value.

.. _Generator: http://www.sgi.com/tech/stl/Generator.html

The Function Input Iterator encapsulates a state object which models the
`Incrementable Concept`_ and the EqualityComparable_ Concept. These concepts are
described below as:

.. _EqualityComparable: http://www.sgi.com/tech/stl/EqualityComparable.html

Incrementable Concept
---------------------

A type models the Incrementable Concept when it supports the pre- and post-
increment operators. For a given object ``i`` with type ``I``, the following 
constructs should be valid:

=========  =================  ===========
Construct  Description        Return Type
-----------------------------------------
i++        Post-increment i.  I
++i        Pre-increment i.   I&
=========  =================  ===========

NOTE: An Incrementable type should also be DefaultConstructible_.

.. _DefaultConstructible: http://www.sgi.com/tech/stl/DefaultConstructible.html

Synopsis
--------

::

    namespace {
        template <class Function, class State>
        class function_input_iterator;

        template <class Function, class State>
        typename function_input_iterator<Function, State>
        make_function_input_iterator(Function & f, State s);

        struct infinite;
    }

Function Input Iterator Class
-----------------------------

The class Function Input Iterator class takes two template parameters
``Function`` and ``State``. These two template parameters tell the
Function Input Iterator the type of the function to encapsulate and
the type of the internal state value to hold.

The ``State`` parameter is important in cases where you want to
control the type of the counter which determines whether two iterators 
are at the same state. This allows for creating a pair of iterators which 
bound the range of the invocations of the encapsulated functions.

Examples
--------

The following example shows how we use the function input iterator class
in cases where we want to create bounded (lazy) generated ranges.

::

    struct generator {
        typedef int result_type;
        generator() { srand(time(0)); }
        result_type operator() () const {
            return rand();
        }
    };

    int main(int argc, char * argv[]) {
        generator f;
        copy(
                make_function_input_iterator(f, 0),
                make_function_input_iterator(f, 10),
                ostream_iterator<int>(cout, " ")
            );
        return 0;
    }

Here we can see that we've bounded the number of invocations using an ``int``
that counts from ``0`` to ``10``. Say we want to create an endless stream
of random numbers and encapsulate that in a pair of integers, we can do
it with the ``boost::infinite`` helper class.

::

    copy(
            make_function_input_iterator(f,infinite()),
            make_function_input_iterator(f,infinite()),
            ostream_iterator<int>(cout, " ")
        );
   
Above, instead of creating a huge vector we rely on the STL copy algorithm
to traverse the function input iterator and call the function object f
as it increments the iterator. The special property of ``boost::infinite``
is that equating two instances always yield false -- and that incrementing
an instance of ``boost::infinite`` doesn't do anything. This is an efficient
way of stating that the iterator range provided by two iterators with an
encapsulated infinite state will definitely be infinite.


