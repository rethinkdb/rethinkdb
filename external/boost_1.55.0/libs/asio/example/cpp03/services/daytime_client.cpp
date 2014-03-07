//
// daytime_client.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include "logger.hpp"
#include "stream_socket_service.hpp"

typedef boost::asio::basic_stream_socket<boost::asio::ip::tcp,
    services::stream_socket_service<boost::asio::ip::tcp> > debug_stream_socket;

char read_buffer[1024];

void read_handler(const boost::system::error_code& e,
    std::size_t bytes_transferred, debug_stream_socket* s)
{
  if (!e)
  {
    std::cout.write(read_buffer, bytes_transferred);

    s->async_read_some(boost::asio::buffer(read_buffer),
        boost::bind(read_handler, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred, s));
  }
}

void connect_handler(const boost::system::error_code& e, debug_stream_socket* s)
{
  if (!e)
  {
    s->async_read_some(boost::asio::buffer(read_buffer),
        boost::bind(read_handler, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred, s));
  }
  else
  {
    std::cerr << e.message() << std::endl;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: daytime_client <host>" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;

    // Set the name of the file that all logger instances will use.
    services::logger logger(io_service, "");
    logger.use_file("log.txt");

    // Resolve the address corresponding to the given host.
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(argv[1], "daytime");
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    // Start an asynchronous connect.
    debug_stream_socket socket(io_service);
    boost::asio::async_connect(socket, iterator,
        boost::bind(connect_handler,
          boost::asio::placeholders::error, &socket));

    // Run the io_service until all operations have finished.
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
