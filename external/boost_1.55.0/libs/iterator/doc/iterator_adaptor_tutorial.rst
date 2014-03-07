.. Copyright David Abrahams 2004. Use, modification and distribution is
.. subject to the Boost Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

In this section we'll further refine the ``node_iter`` class
template we developed in the |fac_tut|_.  If you haven't already
read that material, you should go back now and check it out because
we're going to pick up right where it left off.

.. |fac_tut| replace:: ``iterator_facade`` tutorial
.. _fac_tut: iterator_facade.html#tutorial-example

.. sidebar:: ``node_base*`` really *is* an iterator

   It's not really a very interesting iterator, since ``node_base``
   is an abstract class: a pointer to a ``node_base`` just points
   at some base subobject of an instance of some other class, and
   incrementing a ``node_base*`` moves it past this base subobject
   to who-knows-where?  The most we can do with that incremented
   position is to compare another ``node_base*`` to it.  In other
   words, the original iterator traverses a one-element array.

You probably didn't think of it this way, but the ``node_base*``
object that underlies ``node_iterator`` is itself an iterator,
just like all other pointers.  If we examine that pointer closely
from an iterator perspective, we can see that it has much in common
with the ``node_iterator`` we're building.  First, they share most
of the same associated types (``value_type``, ``reference``,
``pointer``, and ``difference_type``).  Second, even some of the
core functionality is the same: ``operator*`` and ``operator==`` on
the ``node_iterator`` return the result of invoking the same
operations on the underlying pointer, via the ``node_iterator``\ 's
|dereference_and_equal|_).  The only real behavioral difference
between ``node_base*`` and ``node_iterator`` can be observed when
they are incremented: ``node_iterator`` follows the
``m_next`` pointer, while ``node_base*`` just applies an address offset.   

.. |dereference_and_equal| replace:: ``dereference`` and ``equal`` member functions
.. _dereference_and_equal: iterator_facade.html#implementing-the-core-operations

It turns out that the pattern of building an iterator on another
iterator-like type (the ``Base`` [#base]_ type) while modifying
just a few aspects of the underlying type's behavior is an
extremely common one, and it's the pattern addressed by
``iterator_adaptor``.  Using ``iterator_adaptor`` is very much like
using ``iterator_facade``, but because iterator_adaptor tries to
mimic as much of the ``Base`` type's behavior as possible, we
neither have to supply a ``Value`` argument, nor implement any core
behaviors other than ``increment``.  The implementation of
``node_iter`` is thus reduced to::

  template <class Value>
  class node_iter
    : public boost::iterator_adaptor<
          node_iter<Value>                // Derived
        , Value*                          // Base
        , boost::use_default              // Value
        , boost::forward_traversal_tag    // CategoryOrTraversal
      >
  {
   private:
      struct enabler {};  // a private type avoids misuse

   public:
      node_iter()
        : node_iter::iterator_adaptor_(0) {}

      explicit node_iter(Value* p)
        : node_iter::iterator_adaptor_(p) {}

      template <class OtherValue>
      node_iter(
          node_iter<OtherValue> const& other
        , typename boost::enable_if<
              boost::is_convertible<OtherValue*,Value*>
            , enabler
          >::type = enabler()
      )
        : node_iter::iterator_adaptor_(other.base()) {}

   private:
      friend class boost::iterator_core_access;
      void increment() { this->base_reference() = this->base()->next(); }
  };

Note the use of ``node_iter::iterator_adaptor_`` here: because
``iterator_adaptor`` defines a nested ``iterator_adaptor_`` type
that refers to itself, that gives us a convenient way to refer to
the complicated base class type of ``node_iter<Value>``. [Note:
this technique is known not to work with Borland C++ 5.6.4 and
Metrowerks CodeWarrior versions prior to 9.0]

You can see an example program that exercises this version of the
node iterators `here`__.

__ ../example/node_iterator3.cpp

In the case of ``node_iter``, it's not very compelling to pass
``boost::use_default`` as ``iterator_adaptor``\ 's ``Value``
argument; we could have just passed ``node_iter``\ 's ``Value``
along to ``iterator_adaptor``, and that'd even be shorter!  Most
iterator class templates built with ``iterator_adaptor`` are
parameterized on another iterator type, rather than on its
``value_type``.  For example, ``boost::reverse_iterator`` takes an
iterator type argument and reverses its direction of traversal,
since the original iterator and the reversed one have all the same
associated types, ``iterator_adaptor``\ 's delegation of default
types to its ``Base`` saves the implementor of
``boost::reverse_iterator`` from writing:

.. parsed-literal::

   std::iterator_traits<Iterator>::*some-associated-type*

at least four times.  

We urge you to review the documentation and implementations of
|reverse_iterator|_ and the other Boost `specialized iterator
adaptors`__ to get an idea of the sorts of things you can do with
``iterator_adaptor``.  In particular, have a look at
|transform_iterator|_, which is perhaps the most straightforward
adaptor, and also |counting_iterator|_, which demonstrates that
``iterator_adaptor``\ 's ``Base`` type needn't be an iterator.

.. |reverse_iterator| replace:: ``reverse_iterator``
.. _reverse_iterator: reverse_iterator.html

.. |counting_iterator| replace:: ``counting_iterator``
.. _counting_iterator: counting_iterator.html

.. |transform_iterator| replace:: ``transform_iterator``
.. _transform_iterator: transform_iterator.html

__ index.html#specialized-adaptors

