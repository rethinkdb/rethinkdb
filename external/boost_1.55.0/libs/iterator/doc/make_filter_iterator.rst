.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

    template <class Predicate, class Iterator>
    filter_iterator<Predicate,Iterator>
    make_filter_iterator(Predicate f, Iterator x, Iterator end = Iterator());

:Returns: filter_iterator<Predicate,Iterator>(f, x, end)

::

    template <class Predicate, class Iterator>
    filter_iterator<Predicate,Iterator>
    make_filter_iterator(Iterator x, Iterator end = Iterator());

:Returns: filter_iterator<Predicate,Iterator>(x, end)
