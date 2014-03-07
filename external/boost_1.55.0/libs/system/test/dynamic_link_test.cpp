//  dynamic_link_test.cpp  -------------------------------------------------------------//

//  Copyright Beman Dawes 2010

//  Distributed under the Boost Software License, Version 1.0.
//  See www.boost.org/LICENSE_1_0.txt

//  Library home page is www.boost.org/libs/system

//--------------------------------------------------------------------------------------// 

//  Dynamic link libraries (DLL's), also know as dynamic shared objects (DSO's),
//  can cause symbol visability problems unless carefully configured. One of the
//  manifestations, particularly with GCC, is that a system_error exception thrown from
//  a DLL or DSO is not caught.
//
//  The purpose of this program is to test for that error.

//--------------------------------------------------------------------------------------// 

#include <boost/system/system_error.hpp>

#include <iostream>

namespace boost
{
  namespace system
  {
    BOOST_SYSTEM_DECL void throw_test();
  }
}

int main()
{
  try
  {
    boost::system::throw_test();
  }
  catch (const boost::system::system_error& ex)
  {
    std::cout << "  caught boost::system::system_error as expected\n";
    std::cout << "  what() reports " << ex.what() << '\n';
    return 0;
  }

  catch (const std::runtime_error& ex)
  {
    std::cout << "  error: caught std::runtime_error instead of boost::system::system_error\n";
    std::cout << "  what() reports " << ex.what() << '\n';
    return 1;
  }

  std::cout << "  error: failed to catch boost::system::system_error\n";
  return 1;
}