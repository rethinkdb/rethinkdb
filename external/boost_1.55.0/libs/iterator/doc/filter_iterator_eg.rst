.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Example
.......

This example uses ``filter_iterator`` and then
``make_filter_iterator`` to output only the positive integers from an
array of integers. Then ``make_filter_iterator`` is is used to output
the integers greater than ``-2``.

::

    struct is_positive_number {
      bool operator()(int x) { return 0 < x; }
    };

    int main()
    {
      int numbers_[] = { 0, -1, 4, -3, 5, 8, -2 };
      const int N = sizeof(numbers_)/sizeof(int);

      typedef int* base_iterator;
      base_iterator numbers(numbers_);

      // Example using filter_iterator
      typedef boost::filter_iterator<is_positive_number, base_iterator>
	FilterIter;

      is_positive_number predicate;
      FilterIter filter_iter_first(predicate, numbers, numbers + N);
      FilterIter filter_iter_last(predicate, numbers + N, numbers + N);

      std::copy(filter_iter_first, filter_iter_last, std::ostream_iterator<int>(std::cout, " "));
      std::cout << std::endl;

      // Example using make_filter_iterator()
      std::copy(boost::make_filter_iterator<is_positive_number>(numbers, numbers + N),
		boost::make_filter_iterator<is_positive_number>(numbers + N, numbers + N),
		std::ostream_iterator<int>(std::cout, " "));
      std::cout << std::endl;

      // Another example using make_filter_iterator()
      std::copy(
	  boost::make_filter_iterator(
	      std::bind2nd(std::greater<int>(), -2)
	    , numbers, numbers + N)

	, boost::make_filter_iterator(
	      std::bind2nd(std::greater<int>(), -2)
	    , numbers + N, numbers + N)

	, std::ostream_iterator<int>(std::cout, " ")
      );

      std::cout << std::endl;

      return boost::exit_success;
    }


The output is::

    4 5 8 
    4 5 8 
    0 -1 4 5 8 


The source code for this example can be found `here`__.

__ ../example/filter_iterator_example.cpp
