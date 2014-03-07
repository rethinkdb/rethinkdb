// Copyright 2003-2008 Jan Gaspar.
// Copyright 2013 Paul A. Bristow.   Added some Quickbook snippet markers.

// Distributed under the Boost Software License, Version 1.0.
// (See the accompanying file LICENSE_1_0.txt
// or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//[circular_buffer_example_1
/*`For all examples, we need this include:
*/

#include <boost/circular_buffer.hpp>

//] [/circular_buffer_example_1]

 int main()
 {

//[circular_buffer_example_2
    // Create a circular buffer with a capacity for 3 integers.
    boost::circular_buffer<int> cb(3);

    // Insert threee elements into the buffer.
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    int a = cb[0];  // a == 1
    int b = cb[1];  // b == 2
    int c = cb[2];  // c == 3

    // The buffer is full now, so pushing subsequent
    // elements will overwrite the front-most elements.

    cb.push_back(4);  // Overwrite 1 with 4.
    cb.push_back(5);  // Overwrite 2 with 5.

    // The buffer now contains 3, 4 and 5.
    a = cb[0];  // a == 3
    b = cb[1];  // b == 4
    c = cb[2];  // c == 5

    // Elements can be popped from either the front or the back.
    cb.pop_back();  // 5 is removed.
    cb.pop_front(); // 3 is removed.

    // Leaving only one element with value = 4.
    int d = cb[0];  // d == 4

//] [/circular_buffer_example_2]
    return 0;
 }

 /*

 //[circular_buffer_example_output

There is no output from this example.

//] [/circular_buffer_example_output]

 */

