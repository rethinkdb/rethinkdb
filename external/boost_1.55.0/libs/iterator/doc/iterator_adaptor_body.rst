.. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. Version 1.2 of this ReStructuredText document corresponds to
   n1530_, the paper accepted by the LWG for TR1.

.. Copyright David Abrahams, Jeremy Siek, and Thomas Witt 2003. 

The ``iterator_adaptor`` class template adapts some ``Base`` [#base]_
type to create a new iterator.  Instantiations of ``iterator_adaptor``
are derived from a corresponding instantiation of ``iterator_facade``
and implement the core behaviors in terms of the ``Base`` type. In
essence, ``iterator_adaptor`` merely forwards all operations to an
instance of the ``Base`` type, which it stores as a member.

.. [#base] The term "Base" here does not refer to a base class and is
   not meant to imply the use of derivation. We have followed the lead
   of the standard library, which provides a base() function to access
   the underlying iterator object of a ``reverse_iterator`` adaptor.

The user of ``iterator_adaptor`` creates a class derived from an
instantiation of ``iterator_adaptor`` and then selectively
redefines some of the core member functions described in the
``iterator_facade`` core requirements table. The ``Base`` type need
not meet the full requirements for an iterator; it need only
support the operations used by the core interface functions of
``iterator_adaptor`` that have not been redefined in the user's
derived class.

Several of the template parameters of ``iterator_adaptor`` default
to ``use_default``. This allows the
user to make use of a default parameter even when she wants to
specify a parameter later in the parameter list.  Also, the
defaults for the corresponding associated types are somewhat
complicated, so metaprogramming is required to compute them, and
``use_default`` can help to simplify the implementation.  Finally,
the identity of the ``use_default`` type is not left unspecified
because specification helps to highlight that the ``Reference``
template parameter may not always be identical to the iterator's
``reference`` type, and will keep users from making mistakes based on
that assumption.

