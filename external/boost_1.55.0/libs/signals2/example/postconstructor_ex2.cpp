// An example of defining a postconstructor for a class which
// uses boost::signals2::deconstruct as its factory function.
// This example expands on the basic postconstructor_ex1.cpp example
// by passing arguments to the constructor and postconstructor.
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
#include <sstream>
#include <string>

namespace bs2 = boost::signals2;

namespace mynamespace
{
  class Y
  {
  public:
    /* This adl_postconstruct function will be found
    via argument-dependent lookup when using boost::signals2::deconstruct. */
    template<typename T> friend
      void adl_postconstruct(const boost::shared_ptr<T> &, Y *y, const std::string &text)
    {
      y->_text_stream << text;
    }
    void print() const
    {
      std::cout << _text_stream.str() << std::endl;
    }
  private:
    friend class bs2::deconstruct_access;  // give boost::signals2::deconstruct access to private constructor
    // private constructor forces use of boost::signals2::deconstruct to create objects.
    Y(const std::string &text)
    {
      _text_stream << text;
    }

    std::ostringstream _text_stream;
  };
}

int main()
{
  boost::shared_ptr<mynamespace::Y> y = bs2::deconstruct<mynamespace::Y>("Hello, ").postconstruct("world!");
  y->print();
  return 0;
}
