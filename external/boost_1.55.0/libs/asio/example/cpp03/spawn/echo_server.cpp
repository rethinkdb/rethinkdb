//
// echo_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <iostream>

using boost::asio::ip::tcp;

class session : public boost::enable_shared_from_this<session>
{
public:
  explicit session(boost::asio::io_service& io_service)
    : strand_(io_service),
      socket_(io_service),
      timer_(io_service)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void go()
  {
    boost::asio::spawn(strand_,
        boost::bind(&session::echo,
          shared_from_this(), _1));
    boost::asio::spawn(strand_,
        boost::bind(&session::timeout,
          shared_from_this(), _1));
  }

private:
  void echo(boost::asio::yield_context yield)
  {
    try
    {
      char data[128];
      for (;;)
      {
        timer_.expires_from_now(boost::posix_time::seconds(10));
        std::size_t n = socket_.async_read_some(boost::asio::buffer(data), yield);
        boost::asio::async_write(socket_, boost::asio::buffer(data, n), yield);
      }
    }
    catch (std::exception& e)
    {
      socket_.close();
      timer_.cancel();
    }
  }

  void timeout(boost::asio::yield_context yield)
  {
    while (socket_.is_open())
    {
      boost::system::error_code ignored_ec;
      timer_.async_wait(yield[ignored_ec]);
      if (timer_.expires_from_now() <= boost::posix_time::seconds(0))
        socket_.close();
    }
  }

  boost::asio::io_service::strand strand_;
  tcp::socket socket_;
  boost::asio::deadline_timer timer_;
};

void do_accept(boost::asio::io_service& io_service,
    unsigned short port, boost::asio::yield_context yield)
{
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));

  for (;;)
  {
    boost::system::error_code ec;
    boost::shared_ptr<session> new_session(new session(io_service));
    acceptor.async_accept(new_session->socket(), yield[ec]);
    if (!ec) new_session->go();
  }
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: echo_server <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    boost::asio::spawn(io_service,
        boost::bind(do_accept,
          boost::ref(io_service), atoi(argv[1]), _1));

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
