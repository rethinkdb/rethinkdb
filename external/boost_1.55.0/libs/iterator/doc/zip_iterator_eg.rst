.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Examples
........

There are two main types of applications of the ``zip_iterator``. The first
one concerns runtime efficiency: If one has several controlled sequences
of the same length that must be somehow processed, e.g., with the 
``for_each`` algorithm, then it is more efficient to perform just
one parallel-iteration rather than several individual iterations. For an 
example, assume that ``vect_of_doubles`` and ``vect_of_ints``
are two vectors of equal length containing doubles and ints, respectively,
and consider the following two iterations:

::


    std::vector<double>::const_iterator beg1 = vect_of_doubles.begin();
    std::vector<double>::const_iterator end1 = vect_of_doubles.end();
    std::vector<int>::const_iterator beg2 = vect_of_ints.begin();
    std::vector<int>::const_iterator end2 = vect_of_ints.end();

    std::for_each(beg1, end1, func_0());
    std::for_each(beg2, end2, func_1());

These two iterations can now be replaced with a single one as follows:

::


    std::for_each(
      boost::make_zip_iterator(
        boost::make_tuple(beg1, beg2)
        ),
      boost::make_zip_iterator(
        boost::make_tuple(end1, end2)
        ),
      zip_func()
      );

A non-generic implementation of ``zip_func`` could look as follows:

::


      struct zip_func : 
        public std::unary_function<const boost::tuple<const double&, const int&>&, void>
      {
        void operator()(const boost::tuple<const double&, const int&>& t) const
        {
          m_f0(t.get<0>());
          m_f1(t.get<1>());
        }

      private:
        func_0 m_f0;
        func_1 m_f1;
      };

The second important application of the ``zip_iterator`` is as a building block
to make combining iterators. A combining iterator is an iterator
that parallel-iterates over several controlled sequences and, upon
dereferencing, returns the result of applying a functor to the values of the
sequences at the respective positions. This can now be achieved by using the
``zip_iterator`` in conjunction with the ``transform_iterator``. 

Suppose, for example, that you have two vectors of doubles, say 
``vect_1`` and ``vect_2``, and you need to expose to a client
a controlled sequence containing the products of the elements of 
``vect_1`` and ``vect_2``. Rather than placing these products
in a third vector, you can use a combining iterator that calculates the
products on the fly. Let us assume that ``tuple_multiplies`` is a
functor that works like ``std::multiplies``, except that it takes
its two arguments packaged in a tuple. Then the two iterators 
``it_begin`` and ``it_end`` defined below delimit a controlled
sequence containing the products of the elements of ``vect_1`` and
``vect_2``:

::


    typedef boost::tuple<
      std::vector<double>::const_iterator,
      std::vector<double>::const_iterator
      > the_iterator_tuple;

    typedef boost::zip_iterator<
      the_iterator_tuple
      > the_zip_iterator;

    typedef boost::transform_iterator<
      tuple_multiplies<double>,
      the_zip_iterator
      > the_transform_iterator;

    the_transform_iterator it_begin(
      the_zip_iterator(
        the_iterator_tuple(
          vect_1.begin(),
          vect_2.begin()
          )
        ),
      tuple_multiplies<double>()
      );

    the_transform_iterator it_end(
      the_zip_iterator(
        the_iterator_tuple(
          vect_1.end(),
          vect_2.end()
          )
        ),
      tuple_multiplies<double>()
      );
