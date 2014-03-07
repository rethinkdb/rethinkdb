.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

    template <class Incrementable>
    counting_iterator<Incrementable> make_counting_iterator(Incrementable x);

:Returns: An instance of ``counting_iterator<Incrementable>``
    with ``current`` constructed from ``x``.

