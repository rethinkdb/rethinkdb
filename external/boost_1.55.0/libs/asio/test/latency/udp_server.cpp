//
// udp_server.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "allocator.hpp"

using boost::asio::ip::udp;

#include "yield.hpp"

class udp_server : coroutine
{
public:
  udp_server(boost::asio::io_service& io_service,
      unsigned short port, std::size_t buf_size) :
    socket_(io_service, udp::endpoint(udp::v4(), port)),
    buffer_(buf_size)
  {
  }

  void operator()(boost::system::error_code ec, std::size_t n = 0)
  {
    reenter (this) for (;;)
    {
      yield socket_.async_receive_from(
          boost::asio::buffer(buffer_),
          sender_, ref(this));

      if (!ec)
      {
        for (std::size_t i = 0; i < n; ++i) buffer_[i] = ~buffer_[i];
        socket_.send_to(boost::asio::buffer(buffer_, n), sender_, 0, ec);
      }
    }
  }

  friend void* asio_handler_allocate(std::size_t n, udp_server* s)
  {
    return s->allocator_.allocate(n);
  }

  friend void asio_handler_deallocate(void* p, std::size_t, udp_server* s)
  {
    s->allocator_.deallocate(p);
  }

  struct ref
  {
    explicit ref(udp_server* p)
      : p_(p)
    {
    }

    void operator()(boost::system::error_code ec, std::size_t n = 0)
    {
      (*p_)(ec, n);
    }

  private:
    udp_server* p_;

    friend void* asio_handler_allocate(std::size_t n, ref* r)
    {
      return asio_handler_allocate(n, r->p_);
    }

    friend void asio_handler_deallocate(void* p, std::size_t n, ref* r)
    {
      asio_handler_deallocate(p, n, r->p_);
    }
  };

private:
  udp::socket socket_;
  std::vector<unsigned char> buffer_;
  udp::endpoint sender_;
  allocator allocator_;
};

#include "unyield.hpp"

int main(int argc, char* argv[])
{
  if (argc != 5)
  {
    std::fprintf(stderr,
        "Usage: udp_server <port1> <nports> "
        "<bufsize> {spin|block}\n");
    return 1;
  }

  unsigned short first_port = static_cast<unsigned short>(std::atoi(argv[1]));
  unsigned short num_ports = static_cast<unsigned short>(std::atoi(argv[2]));
  std::size_t buf_size = std::atoi(argv[3]);
  bool spin = (std::strcmp(argv[4], "spin") == 0);

  boost::asio::io_service io_service(1);
  std::vector<boost::shared_ptr<udp_server> > servers;

  for (unsigned short i = 0; i < num_ports; ++i)
  {
    unsigned short port = first_port + i;
    boost::shared_ptr<udp_server> s(new udp_server(io_service, port, buf_size));
    servers.push_back(s);
    (*s)(boost::system::error_code());
  }

  if (spin)
    for (;;) io_service.poll();
  else
    io_service.run();
}
