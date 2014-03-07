//  filesystem tut6c.cpp  --------------------------------------------------------------//

//  Copyright Beman Dawes 2010

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#include <iostream>
#include <exception>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

using namespace boost::filesystem;
using namespace boost::system;

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: tut6c path\n";
    return 1;
  }

  error_code ec;
  for (recursive_directory_iterator it (argv[1], ec);
        it != recursive_directory_iterator();
      )
  {
    for (int i = 0; i <= it.level(); ++i)
      std::cout << "  ";

    std::cout << it->path() << '\n';

    it.increment(ec);
  }

  return 0;
}  
