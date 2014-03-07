.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. Version 1.1 of this ReStructuredText document corresponds to
   n1530_, the paper accepted by the LWG for TR1.

.. Copyright David Abrahams, Jeremy Siek, and Thomas Witt 2003. 


While the iterator interface is rich, there is a core subset of the
interface that is necessary for all the functionality.  We have
identified the following core behaviors for iterators:

* dereferencing
* incrementing
* decrementing
* equality comparison
* random-access motion
* distance measurement

In addition to the behaviors listed above, the core interface elements
include the associated types exposed through iterator traits:
``value_type``, ``reference``, ``difference_type``, and
``iterator_category``.

Iterator facade uses the Curiously Recurring Template
Pattern (CRTP) [Cop95]_ so that the user can specify the behavior
of ``iterator_facade`` in a derived class.  Former designs used
policy objects to specify the behavior, but that approach was
discarded for several reasons:

  1. the creation and eventual copying of the policy object may create
     overhead that can be avoided with the current approach.

  2. The policy object approach does not allow for custom constructors
     on the created iterator types, an essential feature if
     ``iterator_facade`` should be used in other library
     implementations.

  3. Without the use of CRTP, the standard requirement that an
     iterator's ``operator++`` returns the iterator type itself
     would mean that all iterators built with the library would
     have to be specializations of ``iterator_facade<...>``, rather
     than something more descriptive like
     ``indirect_iterator<T*>``.  Cumbersome type generator
     metafunctions would be needed to build new parameterized
     iterators, and a separate ``iterator_adaptor`` layer would be
     impossible.

Usage
-----

The user of ``iterator_facade`` derives his iterator class from a
specialization of ``iterator_facade`` and passes the derived
iterator class as ``iterator_facade``\ 's first template parameter.
The order of the other template parameters have been carefully
chosen to take advantage of useful defaults.  For example, when
defining a constant lvalue iterator, the user can pass a
const-qualified version of the iterator's ``value_type`` as
``iterator_facade``\ 's ``Value`` parameter and omit the
``Reference`` parameter which follows.

The derived iterator class must define member functions implementing
the iterator's core behaviors.  The following table describes
expressions which are required to be valid depending on the category
of the derived iterator type.  These member functions are described
briefly below and in more detail in the iterator facade
requirements.

   +------------------------+-------------------------------+
   |Expression              |Effects                        |
   +========================+===============================+
   |``i.dereference()``     |Access the value referred to   |
   +------------------------+-------------------------------+
   |``i.equal(j)``          |Compare for equality with ``j``|
   +------------------------+-------------------------------+
   |``i.increment()``       |Advance by one position        |
   +------------------------+-------------------------------+
   |``i.decrement()``       |Retreat by one position        |
   +------------------------+-------------------------------+
   |``i.advance(n)``        |Advance by ``n`` positions     |
   +------------------------+-------------------------------+
   |``i.distance_to(j)``    |Measure the distance to ``j``  |
   +------------------------+-------------------------------+

.. Should we add a comment that a zero overhead implementation of iterator_facade
   is possible with proper inlining?

In addition to implementing the core interface functions, an iterator
derived from ``iterator_facade`` typically defines several
constructors. To model any of the standard iterator concepts, the
iterator must at least have a copy constructor. Also, if the iterator
type ``X`` is meant to be automatically interoperate with another
iterator type ``Y`` (as with constant and mutable iterators) then
there must be an implicit conversion from ``X`` to ``Y`` or from ``Y``
to ``X`` (but not both), typically implemented as a conversion
constructor. Finally, if the iterator is to model Forward Traversal
Iterator or a more-refined iterator concept, a default constructor is
required.



Iterator Core Access
--------------------

``iterator_facade`` and the operator implementations need to be able
to access the core member functions in the derived class.  Making the
core member functions public would expose an implementation detail to
the user.  The design used here ensures that implementation details do
not appear in the public interface of the derived iterator type.

Preventing direct access to the core member functions has two
advantages.  First, there is no possibility for the user to accidently
use a member function of the iterator when a member of the value_type
was intended.  This has been an issue with smart pointer
implementations in the past.  The second and main advantage is that
library implementers can freely exchange a hand-rolled iterator
implementation for one based on ``iterator_facade`` without fear of
breaking code that was accessing the public core member functions
directly.

In a naive implementation, keeping the derived class' core member
functions private would require it to grant friendship to
``iterator_facade`` and each of the seven operators.  In order to
reduce the burden of limiting access, ``iterator_core_access`` is
provided, a class that acts as a gateway to the core member functions
in the derived iterator class.  The author of the derived class only
needs to grant friendship to ``iterator_core_access`` to make his core
member functions available to the library.

.. This is no long uptodate -thw 
.. Yes it is; I made sure of it! -DWA

``iterator_core_access`` will be typically implemented as an empty
class containing only private static member functions which invoke the
iterator core member functions. There is, however, no need to
standardize the gateway protocol.  Note that even if
``iterator_core_access`` used public member functions it would not
open a safety loophole, as every core member function preserves the
invariants of the iterator.

``operator[]``
--------------

The indexing operator for a generalized iterator presents special
challenges.  A random access iterator's ``operator[]`` is only
required to return something convertible to its ``value_type``.
Requiring that it return an lvalue would rule out currently-legal
random-access iterators which hold the referenced value in a data
member (e.g. |counting|_), because ``*(p+n)`` is a reference
into the temporary iterator ``p+n``, which is destroyed when
``operator[]`` returns.

.. |counting| replace:: ``counting_iterator``

Writable iterators built with ``iterator_facade`` implement the
semantics required by the preferred resolution to `issue 299`_ and
adopted by proposal n1550_: the result of ``p[n]`` is an object
convertible to the iterator's ``value_type``, and ``p[n] = x`` is
equivalent to ``*(p + n) = x`` (Note: This result object may be
implemented as a proxy containing a copy of ``p+n``).  This approach
will work properly for any random-access iterator regardless of the
other details of its implementation.  A user who knows more about
the implementation of her iterator is free to implement an
``operator[]`` that returns an lvalue in the derived iterator
class; it will hide the one supplied by ``iterator_facade`` from
clients of her iterator.

.. _n1550: http://anubis.dkuug.dk/JTC1/SC22/WG21/docs/papers/2003/n1550.html

.. _`issue 299`: http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/lwg-active.html#299

.. _`operator arrow`:


``operator->``
--------------

The ``reference`` type of a readable iterator (and today's input
iterator) need not in fact be a reference, so long as it is
convertible to the iterator's ``value_type``.  When the ``value_type``
is a class, however, it must still be possible to access members
through ``operator->``.  Therefore, an iterator whose ``reference``
type is not in fact a reference must return a proxy containing a copy
of the referenced value from its ``operator->``.

The return types for ``iterator_facade``\ 's ``operator->`` and
``operator[]`` are not explicitly specified. Instead, those types
are described in terms of a set of requirements, which must be
satisfied by the ``iterator_facade`` implementation.

.. [Cop95] [Coplien, 1995] Coplien, J., Curiously Recurring Template
   Patterns, C++ Report, February 1995, pp. 24-27.

