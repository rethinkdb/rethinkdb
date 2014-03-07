//
// tcp_server.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using boost::asio::ip::tcp;

#include "yield.hpp"

class tcp_server : coroutine
{
public:
  tcp_server(tcp::acceptor& acceptor, std::size_t buf_size) :
    acceptor_(acceptor),
    socket_(acceptor_.get_io_service()),
    buffer_(buf_size)
  {
  }

  void operator()(boost::system::error_code ec, std::size_t n = 0)
  {
    reenter (this) for (;;)
    {
      yield acceptor_.async_accept(socket_, ref(this));

      while (!ec)
      {
        yield boost::asio::async_read(socket_,
            boost::asio::buffer(buffer_), ref(this));

        if (!ec)
        {
          for (std::size_t i = 0; i < n; ++i) buffer_[i] = ~buffer_[i];

          yield boost::asio::async_write(socket_,
              boost::asio::buffer(buffer_), ref(this));
        }
      }

      socket_.close();
    }
  }

  struct ref
  {
    explicit ref(tcp_server* p)
      : p_(p)
    {
    }

    void operator()(boost::system::error_code ec, std::size_t n = 0)
    {
      (*p_)(ec, n);
    }

  private:
    tcp_server* p_;
  };

private:
  tcp::acceptor& acceptor_;
  tcp::socket socket_;
  std::vector<unsigned char> buffer_;
  tcp::endpoint sender_;
};

#include "unyield.hpp"

int main(int argc, char* argv[])
{
  if (argc != 5)
  {
    std::fprintf(stderr,
        "Usage: tcp_server <port> <nconns> "
        "<bufsize> {spin|block}\n");
    return 1;
  }

  unsigned short port = static_cast<unsigned short>(std::atoi(argv[1]));
  int max_connections = std::atoi(argv[2]);
  std::size_t buf_size = std::atoi(argv[3]);
  bool spin = (std::strcmp(argv[4], "spin") == 0);

  boost::asio::io_service io_service(1);
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
  std::vector<boost::shared_ptr<tcp_server> > servers;

  for (int i = 0; i < max_connections; ++i)
  {
    boost::shared_ptr<tcp_server> s(new tcp_server(acceptor, buf_size));
    servers.push_back(s);
    (*s)(boost::system::error_code());
  }

  if (spin)
    for (;;) io_service.poll();
  else
    io_service.run();
}
