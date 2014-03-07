.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

    template <class UnaryFunction, class Iterator>
    transform_iterator<UnaryFunction, Iterator>
    make_transform_iterator(Iterator it, UnaryFunction fun);

:Returns: An instance of ``transform_iterator<UnaryFunction, Iterator>`` with ``m_f``
  initialized to ``f`` and ``m_iterator`` initialized to ``x``.



::

    template <class UnaryFunction, class Iterator>
    transform_iterator<UnaryFunction, Iterator>
    make_transform_iterator(Iterator it);

:Returns: An instance of ``transform_iterator<UnaryFunction, Iterator>`` with ``m_f``
  default constructed and ``m_iterator`` initialized to ``x``.
