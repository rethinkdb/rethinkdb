/* simple example for using class array<>
 *
 * (C) Copyright Nicolai M. Josuttis 2001.
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 * 
 * Changelog:
 * 20 Jan 2001 - Removed boolalpha use since stock GCC doesn't support it
 *               (David Abrahams)
 */

#include <iostream>
#include <boost/array.hpp>

int main()
{
    // define special type name
    typedef boost::array<float,6> Array;

    // create and initialize an array
    Array a = { { 42 } };

    // access elements
    for (unsigned i=1; i<a.size(); ++i) {
        a[i] = a[i-1]+1;
    }

    // use some common STL container operations
    std::cout << "size:     " << a.size() << std::endl;
    std::cout << "empty:    " << (a.empty() ? "true" : "false") << std::endl;
    std::cout << "max_size: " << a.max_size() << std::endl;
    std::cout << "front:    " << a.front() << std::endl;
    std::cout << "back:     " << a.back() << std::endl;
    std::cout << "elems:    ";

    // iterate through all elements
    for (Array::const_iterator pos=a.begin(); pos<a.end(); ++pos) {
        std::cout << *pos << ' ';
    }
    std::cout << std::endl;

    // check copy constructor and assignment operator
    Array b(a);
    Array c;
    c = a;
    if (a==b && a==c) {
        std::cout << "copy construction and copy assignment are OK"
                  << std::endl;
    }
    else {
        std::cout << "copy construction and copy assignment FAILED"
                  << std::endl;
    }

    return 0;  // makes Visual-C++ compiler happy
}

