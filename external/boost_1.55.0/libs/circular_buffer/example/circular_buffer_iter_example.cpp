// Copyright 2003-2008 Jan Gaspar.
// Copyright 2013 Paul A. Bristow.   Added some Quickbook snippet markers.

// Distributed under the Boost Software License, Version 1.0.
// (See the accompanying file LICENSE_1_0.txt
// or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//[circular_buffer_iter_example_1
/*`
*/

#define BOOST_CB_DISABLE_DEBUG // The Debug Support has to be disabled, otherwise the code produces a runtime error.

#include <boost/circular_buffer.hpp>
#include <boost/assert.hpp>
#include <assert.h>

int main(int /*argc*/, char* /*argv*/[])
{

  boost::circular_buffer<int> cb(3);

  cb.push_back(1);
  cb.push_back(2);
  cb.push_back(3);

  boost::circular_buffer<int>::iterator it = cb.begin();

  assert(*it == 1);

  cb.push_back(4);

  assert(*it == 4); // The iterator still points to the initialized memory.

  return 0;
}
   
 //] [/circular_buffer_iter_example_1]
