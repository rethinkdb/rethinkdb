//  Boost shared_ptr_example2 header file  -----------------------------------//

//  Copyright Beman Dawes 2001.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


//  See http://www.boost.org/libs/smart_ptr for documentation.

#include <boost/shared_ptr.hpp>

//  This example demonstrates the handle/body idiom (also called pimpl and
//  several other names).  It separates the interface (in this header file)
//  from the implementation (in shared_ptr_example2.cpp).

//  Note that even though example::implementation is an incomplete type in
//  some translation units using this header, shared_ptr< implementation >
//  is still valid because the type is complete where it counts - in the
//  shared_ptr_example2.cpp translation unit where functions requiring a
//  complete type are actually instantiated.

class example
{
public:
  example();
  void do_something();
private:
  class implementation;
  boost::shared_ptr< implementation > _imp; // hide implementation details
};

