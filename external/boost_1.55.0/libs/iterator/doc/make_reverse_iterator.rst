.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

  template <class BidirectionalIterator>
  reverse_iterator<BidirectionalIterator>n
  make_reverse_iterator(BidirectionalIterator x);

:Returns: An instance of ``reverse_iterator<BidirectionalIterator>``
  with a ``current`` constructed from ``x``.

