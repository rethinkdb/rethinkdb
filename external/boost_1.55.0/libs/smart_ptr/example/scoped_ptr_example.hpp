//  Boost scoped_ptr_example header file  ------------------------------------//

//  Copyright Beman Dawes 2001.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


//  See http://www.boost.org/libs/smart_ptr for documentation.

#include <boost/utility.hpp>
#include <boost/scoped_ptr.hpp>

//  The point of this example is to prove that even though
//  example::implementation is an incomplete type in translation units using
//  this header, scoped_ptr< implementation > is still valid because the type
//  is complete where it counts - in the inplementation translation unit where
//  destruction is actually instantiated.

class example : private boost::noncopyable
{
 public:
  example();
  ~example();
  void do_something();
 private:
  class implementation;
  boost::scoped_ptr< implementation > _imp; // hide implementation details
};

