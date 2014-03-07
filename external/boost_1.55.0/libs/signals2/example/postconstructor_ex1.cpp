// Minimal example of defining a postconstructor for a class which
// uses boost::signals2::deconstruct as its factory function.
//
// Copyright Frank Mori Hess 2009.

// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <boost/shared_ptr.hpp>
#include <boost/signals2/deconstruct.hpp>
#include <iostream>

namespace bs2 = boost::signals2;

namespace mynamespace
{
  class X
  {
  public:
    /* This adl_postconstruct function will be found
    via argument-dependent lookup when using boost::signals2::deconstruct. */
    template<typename T> friend
      void adl_postconstruct(const boost::shared_ptr<T> &, X *)
    {
      std::cout << "world!" << std::endl;
    }
  private:
    friend class bs2::deconstruct_access;  // give boost::signals2::deconstruct access to private constructor
    // private constructor forces use of boost::signals2::deconstruct to create objects.
    X()
    {
      std::cout << "Hello, ";
    }
  };
}

int main()
{
  // adl_postconstruct will be called during implicit conversion of return value to shared_ptr
  boost::shared_ptr<mynamespace::X> x = bs2::deconstruct<mynamespace::X>();
  return 0;
}
