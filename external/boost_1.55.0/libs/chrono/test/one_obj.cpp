//  boost win32_test.cpp  -----------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.
#include <iostream>
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

void another();

int main()
{
  boost::chrono::steady_clock::time_point t1=boost::chrono::steady_clock::now();
  another();
  boost::chrono::steady_clock::time_point t2=boost::chrono::steady_clock::now();
  std::cout << t2-t1 << std::endl;
  return 0;
}

