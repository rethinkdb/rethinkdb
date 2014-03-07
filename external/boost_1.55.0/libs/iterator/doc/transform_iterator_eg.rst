.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Example
.......

This is a simple example of using the transform_iterators class to
generate iterators that multiply (or add to) the value returned by
dereferencing the iterator. It would be cooler to use lambda library
in this example.

::

    int x[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    const int N = sizeof(x)/sizeof(int);

    typedef boost::binder1st< std::multiplies<int> > Function;
    typedef boost::transform_iterator<Function, int*> doubling_iterator;

    doubling_iterator i(x, boost::bind1st(std::multiplies<int>(), 2)),
      i_end(x + N, boost::bind1st(std::multiplies<int>(), 2));

    std::cout << "multiplying the array by 2:" << std::endl;
    while (i != i_end)
      std::cout << *i++ << " ";
    std::cout << std::endl;

    std::cout << "adding 4 to each element in the array:" << std::endl;
    std::copy(boost::make_transform_iterator(x, boost::bind1st(std::plus<int>(), 4)),
	      boost::make_transform_iterator(x + N, boost::bind1st(std::plus<int>(), 4)),
	      std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;


The output is::

    multiplying the array by 2:
    2 4 6 8 10 12 14 16 
    adding 4 to each element in the array:
    5 6 7 8 9 10 11 12


The source code for this example can be found `here`__.

__ ../example/transform_iterator_example.cpp
