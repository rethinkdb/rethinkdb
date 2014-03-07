//  filesystem tut6b.cpp  --------------------------------------------------------------//

//  Copyright Beman Dawes 2010

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#include <iostream>
#include <exception>
#include <boost/filesystem.hpp>
using namespace boost::filesystem;

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: tut6b path\n";
    return 1;
  }

  try
  {
    for (recursive_directory_iterator it (argv[1]);
         it != recursive_directory_iterator();
        )
    {
      for (int i = 0; i <= it.level(); ++i)
        std::cout << "  ";

      std::cout << it->path() << '\n';

      try { ++it; }
      catch (const filesystem_error& ex)
      {
        std::cout << "************* filesystem_error *****************\n";
        std::cout << ex.what() << '\n';
      }
    }
  }
  
  catch (const std::exception& ex)
  {
    std::cout << "************* exception *****************\n";
    std::cout << ex.what() << '\n';
  }

  return 0;
}  
