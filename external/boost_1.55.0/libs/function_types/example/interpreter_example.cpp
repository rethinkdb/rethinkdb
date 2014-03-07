
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <stdexcept>

#include "interpreter.hpp"

void echo(std::string const & s)
{
  std::cout << s << std::endl;
}

void add(int a, int b)
{
  std::cout << a + b << std::endl;
}

void repeat(std::string const & s, int n)
{
  while (--n >= 0) std::cout << s;
  std::cout << std::endl; 
}

int main()
{
  example::interpreter interpreter;

  interpreter.register_function("echo", & echo);
  interpreter.register_function("add", & add);
  interpreter.register_function("repeat", & repeat);

  std::string line = "nonempty";
  while (! line.empty())
  {
    std::cout << std::endl << "] ", std::getline(std::cin,line);

    try                          
    {
      interpreter.parse_input(line);
    }
    catch (std::runtime_error &error) 
    { 
      std::cerr << error.what() << std::endl; 
    }
  }

  return 0;
}

