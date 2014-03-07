//
// reference_counted.cpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <vector>

using boost::asio::ip::tcp;

// A reference-counted non-modifiable buffer class.
class shared_const_buffer
{
public:
  // Construct from a std::string.
  explicit shared_const_buffer(const std::string& data)
    : data_(new std::vector<char>(data.begin(), data.end())),
      buffer_(boost::asio::buffer(*data_))
  {
  }

  // Implement the ConstBufferSequence requirements.
  typedef boost::asio::const_buffer value_type;
  typedef const boost::asio::const_buffer* const_iterator;
  const boost::asio::const_buffer* begin() const { return &buffer_; }
  const boost::asio::const_buffer* end() const { return &buffer_ + 1; }

private:
  boost::shared_ptr<std::vector<char> > data_;
  boost::asio::const_buffer buffer_;
};

class session
  : public boost::enable_shared_from_this<session>
{
public:
  session(boost::asio::io_service& io_service)
    : socket_(io_service)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    using namespace std; // For time_t, time and ctime.
    time_t now = time(0);
    shared_const_buffer buffer(ctime(&now));
    boost::asio::async_write(socket_, buffer,
        boost::bind(&session::handle_write, shared_from_this()));
  }

  void handle_write()
  {
  }

private:
  // The socket used to communicate with the client.
  tcp::socket socket_;
};

typedef boost::shared_ptr<session> session_ptr;

class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
    session_ptr new_session(new session(io_service_));
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(session_ptr new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_session->start();
    }

    new_session.reset(new session(io_service_));
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

private:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: reference_counted <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    server s(io_service, atoi(argv[1]));

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
