//
// echo_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;

class session : public std::enable_shared_from_this<session>
{
public:
  explicit session(tcp::socket socket)
    : socket_(std::move(socket)),
      timer_(socket_.get_io_service()),
      strand_(socket_.get_io_service())
  {
  }

  void go()
  {
    auto self(shared_from_this());
    boost::asio::spawn(strand_,
        [this, self](boost::asio::yield_context yield)
        {
          try
          {
            char data[128];
            for (;;)
            {
              timer_.expires_from_now(std::chrono::seconds(10));
              std::size_t n = socket_.async_read_some(boost::asio::buffer(data), yield);
              boost::asio::async_write(socket_, boost::asio::buffer(data, n), yield);
            }
          }
          catch (std::exception& e)
          {
            socket_.close();
            timer_.cancel();
          }
        });

    boost::asio::spawn(strand_,
        [this, self](boost::asio::yield_context yield)
        {
          while (socket_.is_open())
          {
            boost::system::error_code ignored_ec;
            timer_.async_wait(yield[ignored_ec]);
            if (timer_.expires_from_now() <= std::chrono::seconds(0))
              socket_.close();
          }
        });
  }

private:
  tcp::socket socket_;
  boost::asio::steady_timer timer_;
  boost::asio::io_service::strand strand_;
};

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
        [&](boost::asio::yield_context yield)
        {
          tcp::acceptor acceptor(io_service,
            tcp::endpoint(tcp::v4(), std::atoi(argv[1])));

          for (;;)
          {
            boost::system::error_code ec;
            tcp::socket socket(io_service);
            acceptor.async_accept(socket, yield[ec]);
            if (!ec) std::make_shared<session>(std::move(socket))->go();
          }
        });

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
