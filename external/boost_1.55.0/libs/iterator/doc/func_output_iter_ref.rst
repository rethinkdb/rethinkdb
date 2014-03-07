.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Header
......

::
  
  #include <boost/function_output_iterator.hpp>

::

  template <class UnaryFunction>
  class function_output_iterator {
  public:
    typedef std::output_iterator_tag iterator_category;
    typedef void                     value_type;
    typedef void                     difference_type;
    typedef void                     pointer;
    typedef void                     reference;

    explicit function_output_iterator();

    explicit function_output_iterator(const UnaryFunction& f);

    /* see below */ operator*();
    function_output_iterator& operator++();
    function_output_iterator& operator++(int);
  private:
    UnaryFunction m_f;     // exposition only
  };



``function_output_iterator`` requirements
.........................................

``UnaryFunction`` must be Assignable and Copy Constructible.  



``function_output_iterator`` models
...................................

``function_output_iterator`` is a model of the Writable and
Incrementable Iterator concepts.



``function_output_iterator`` operations
.......................................

``explicit function_output_iterator(const UnaryFunction& f = UnaryFunction());``

:Effects: Constructs an instance of ``function_output_iterator`` 
  with ``m_f`` constructed from ``f``.


``operator*();``

:Returns: An object ``r`` of unspecified type such that ``r = t``
  is equivalent to ``m_f(t)`` for all ``t``.
  

``function_output_iterator& operator++();``

:Returns: ``*this``


``function_output_iterator& operator++(int);``

:Returns: ``*this``
