.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Example
.......

This example prints an array of characters, using
``indirect_iterator`` to access the array of characters through an
array of pointers. Next ``indirect_iterator`` is used with the
``transform`` algorithm to copy the characters (incremented by one) to
another array. A constant indirect iterator is used for the source and
a mutable indirect iterator is used for the destination. The last part
of the example prints the original array of characters, but this time
using the ``make_indirect_iterator`` helper function.


::

    char characters[] = "abcdefg";
    const int N = sizeof(characters)/sizeof(char) - 1; // -1 since characters has a null char
    char* pointers_to_chars[N];                        // at the end.
    for (int i = 0; i < N; ++i)
      pointers_to_chars[i] = &characters[i];

    // Example of using indirect_iterator

    boost::indirect_iterator<char**, char>
      indirect_first(pointers_to_chars), indirect_last(pointers_to_chars + N);

    std::copy(indirect_first, indirect_last, std::ostream_iterator<char>(std::cout, ","));
    std::cout << std::endl;


    // Example of making mutable and constant indirect iterators

    char mutable_characters[N];
    char* pointers_to_mutable_chars[N];
    for (int j = 0; j < N; ++j)
      pointers_to_mutable_chars[j] = &mutable_characters[j];

    boost::indirect_iterator<char* const*> mutable_indirect_first(pointers_to_mutable_chars),
      mutable_indirect_last(pointers_to_mutable_chars + N);
    boost::indirect_iterator<char* const*, char const> const_indirect_first(pointers_to_chars),
      const_indirect_last(pointers_to_chars + N);

    std::transform(const_indirect_first, const_indirect_last,
		   mutable_indirect_first, std::bind1st(std::plus<char>(), 1));

    std::copy(mutable_indirect_first, mutable_indirect_last,
	      std::ostream_iterator<char>(std::cout, ","));
    std::cout << std::endl;


    // Example of using make_indirect_iterator()

    std::copy(boost::make_indirect_iterator(pointers_to_chars), 
	      boost::make_indirect_iterator(pointers_to_chars + N),
	      std::ostream_iterator<char>(std::cout, ","));
    std::cout << std::endl;


The output is::

    a,b,c,d,e,f,g,
    b,c,d,e,f,g,h,
    a,b,c,d,e,f,g,


The source code for this example can be found `here`__.

__ ../example/indirect_iterator_example.cpp

