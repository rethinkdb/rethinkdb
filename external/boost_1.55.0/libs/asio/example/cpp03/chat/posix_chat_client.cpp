//
// posix_chat_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "chat_message.hpp"

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

using boost::asio::ip::tcp;
namespace posix = boost::asio::posix;

class posix_chat_client
{
public:
  posix_chat_client(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator)
    : socket_(io_service),
      input_(io_service, ::dup(STDIN_FILENO)),
      output_(io_service, ::dup(STDOUT_FILENO)),
      input_buffer_(chat_message::max_body_length)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        boost::bind(&posix_chat_client::handle_connect, this,
          boost::asio::placeholders::error));
  }

private:

  void handle_connect(const boost::system::error_code& error)
  {
    if (!error)
    {
      // Read the fixed-length header of the next message from the server.
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&posix_chat_client::handle_read_header, this,
            boost::asio::placeholders::error));

      // Read a line of input entered by the user.
      boost::asio::async_read_until(input_, input_buffer_, '\n',
          boost::bind(&posix_chat_client::handle_read_input, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
  }

  void handle_read_header(const boost::system::error_code& error)
  {
    if (!error && read_msg_.decode_header())
    {
      // Read the variable-length body of the message from the server.
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
          boost::bind(&posix_chat_client::handle_read_body, this,
            boost::asio::placeholders::error));
    }
    else
    {
      close();
    }
  }

  void handle_read_body(const boost::system::error_code& error)
  {
    if (!error)
    {
      // Write out the message we just received, terminated by a newline.
      static char eol[] = { '\n' };
      boost::array<boost::asio::const_buffer, 2> buffers = {{
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        boost::asio::buffer(eol) }};
      boost::asio::async_write(output_, buffers,
          boost::bind(&posix_chat_client::handle_write_output, this,
            boost::asio::placeholders::error));
    }
    else
    {
      close();
    }
  }

  void handle_write_output(const boost::system::error_code& error)
  {
    if (!error)
    {
      // Read the fixed-length header of the next message from the server.
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&posix_chat_client::handle_read_header, this,
            boost::asio::placeholders::error));
    }
    else
    {
      close();
    }
  }

  void handle_read_input(const boost::system::error_code& error,
      std::size_t length)
  {
    if (!error)
    {
      // Write the message (minus the newline) to the server.
      write_msg_.body_length(length - 1);
      input_buffer_.sgetn(write_msg_.body(), length - 1);
      input_buffer_.consume(1); // Remove newline from input.
      write_msg_.encode_header();
      boost::asio::async_write(socket_,
          boost::asio::buffer(write_msg_.data(), write_msg_.length()),
          boost::bind(&posix_chat_client::handle_write, this,
            boost::asio::placeholders::error));
    }
    else if (error == boost::asio::error::not_found)
    {
      // Didn't get a newline. Send whatever we have.
      write_msg_.body_length(input_buffer_.size());
      input_buffer_.sgetn(write_msg_.body(), input_buffer_.size());
      write_msg_.encode_header();
      boost::asio::async_write(socket_,
          boost::asio::buffer(write_msg_.data(), write_msg_.length()),
          boost::bind(&posix_chat_client::handle_write, this,
            boost::asio::placeholders::error));
    }
    else
    {
      close();
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      // Read a line of input entered by the user.
      boost::asio::async_read_until(input_, input_buffer_, '\n',
          boost::bind(&posix_chat_client::handle_read_input, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      close();
    }
  }

  void close()
  {
    // Cancel all outstanding asynchronous operations.
    socket_.close();
    input_.close();
    output_.close();
  }

private:
  tcp::socket socket_;
  posix::stream_descriptor input_;
  posix::stream_descriptor output_;
  chat_message read_msg_;
  chat_message write_msg_;
  boost::asio::streambuf input_buffer_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: posix_chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], argv[2]);
    tcp::resolver::iterator iterator = resolver.resolve(query);

    posix_chat_client c(io_service, iterator);

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int main() {}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
