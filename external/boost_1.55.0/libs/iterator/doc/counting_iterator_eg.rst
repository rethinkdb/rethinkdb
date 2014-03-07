.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Example
.......

This example fills an array with numbers and a second array with
pointers into the first array, using ``counting_iterator`` for both
tasks. Finally ``indirect_iterator`` is used to print out the numbers
into the first array via indirection through the second array.

::

    int N = 7;
    std::vector<int> numbers;
    typedef std::vector<int>::iterator n_iter;
    std::copy(boost::counting_iterator<int>(0),
             boost::counting_iterator<int>(N),
             std::back_inserter(numbers));

    std::vector<std::vector<int>::iterator> pointers;
    std::copy(boost::make_counting_iterator(numbers.begin()),
	      boost::make_counting_iterator(numbers.end()),
	      std::back_inserter(pointers));

    std::cout << "indirectly printing out the numbers from 0 to " 
	      << N << std::endl;
    std::copy(boost::make_indirect_iterator(pointers.begin()),
	      boost::make_indirect_iterator(pointers.end()),
	      std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;


The output is::

    indirectly printing out the numbers from 0 to 7
    0 1 2 3 4 5 6 

The source code for this example can be found `here`__.

__ ../example/counting_iterator_example.cpp

