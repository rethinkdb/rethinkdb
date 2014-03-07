.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

Example
.......

The following example prints an array of characters in reverse order
using ``reverse_iterator``.

::
    
    char letters_[] = "hello world!";
    const int N = sizeof(letters_)/sizeof(char) - 1;
    typedef char* base_iterator;
    base_iterator letters(letters_);
    std::cout << "original sequence of letters:\t\t\t" << letters_ << std::endl;

    boost::reverse_iterator<base_iterator>
      reverse_letters_first(letters + N),
      reverse_letters_last(letters);

    std::cout << "sequence in reverse order:\t\t\t";
    std::copy(reverse_letters_first, reverse_letters_last,
              std::ostream_iterator<char>(std::cout));
    std::cout << std::endl;

    std::cout << "sequence in double-reversed (normal) order:\t";
    std::copy(boost::make_reverse_iterator(reverse_letters_last),
              boost::make_reverse_iterator(reverse_letters_first),
              std::ostream_iterator<char>(std::cout));
    std::cout << std::endl;



The output is::

    original sequence of letters:                   hello world!
    sequence in reverse order:                      !dlrow olleh
    sequence in double-reversed (normal) order:     hello world!


The source code for this example can be found `here`__.

__ ../example/reverse_iterator_example.cpp
