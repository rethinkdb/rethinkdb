.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

    template<typename IteratorTuple> 
    zip_iterator<IteratorTuple> 
    make_zip_iterator(IteratorTuple t);

:Returns: An instance of ``zip_iterator<IteratorTuple>`` with ``m_iterator_tuple``
  initialized to ``t``.
