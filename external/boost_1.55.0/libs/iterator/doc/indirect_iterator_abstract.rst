.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

``indirect_iterator`` adapts an iterator by applying an
*extra* dereference inside of ``operator*()``. For example, this
iterator adaptor makes it possible to view a container of pointers
(e.g. ``list<foo*>``) as if it were a container of the pointed-to type
(e.g. ``list<foo>``).  ``indirect_iterator`` depends on two
auxiliary traits, ``pointee`` and ``indirect_reference``, to
provide support for underlying iterators whose ``value_type`` is
not an iterator.



