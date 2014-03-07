//
// daytime_client.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: daytime_client <host>" << std::endl;
      return 1;
    }

    tcp::iostream s(argv[1], "daytime");
    if (!s)
    {
      std::cout << "Unable to connect: " << s.error().message() << std::endl;
      return 1;
    }

    std::string line;
    std::getline(s, line);
    std::cout << line << std::endl;
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
