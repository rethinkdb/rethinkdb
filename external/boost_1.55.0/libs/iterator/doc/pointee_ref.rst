.. Copyright David Abrahams 2004. Use, modification and distribution is
.. subject to the Boost Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

  template <class Dereferenceable>
  struct pointee
  {
      typedef /* see below */ type;
  };

:Requires: For an object ``x`` of type ``Dereferenceable``, ``*x``
  is well-formed.  If ``++x`` is ill-formed it shall neither be
  ambiguous nor shall it violate access control, and
  ``Dereferenceable::element_type`` shall be an accessible type.
  Otherwise ``iterator_traits<Dereferenceable>::value_type`` shall
  be well formed.  [Note: These requirements need not apply to
  explicit or partial specializations of ``pointee``]

``type`` is determined according to the following algorithm, where
``x`` is an object of type ``Dereferenceable``::

  if ( ++x is ill-formed )
  {
      return ``Dereferenceable::element_type``
  }
  else if (``*x`` is a mutable reference to
           std::iterator_traits<Dereferenceable>::value_type)
  {
      return iterator_traits<Dereferenceable>::value_type
  }
  else
  {
      return iterator_traits<Dereferenceable>::value_type const
  }

  