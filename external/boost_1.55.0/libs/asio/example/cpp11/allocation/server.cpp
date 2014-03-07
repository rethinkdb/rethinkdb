//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

// Class to manage the memory to be used for handler-based custom allocation.
// It contains a single block of memory which may be returned for allocation
// requests. If the memory is in use when an allocation request is made, the
// allocator delegates allocation to the global heap.
class handler_allocator
{
public:
  handler_allocator()
    : in_use_(false)
  {
  }

  handler_allocator(const handler_allocator&) = delete;
  handler_allocator& operator=(const handler_allocator&) = delete;

  void* allocate(std::size_t size)
  {
    if (!in_use_ && size < sizeof(storage_))
    {
      in_use_ = true;
      return &storage_;
    }
    else
    {
      return ::operator new(size);
    }
  }

  void deallocate(void* pointer)
  {
    if (pointer == &storage_)
    {
      in_use_ = false;
    }
    else
    {
      ::operator delete(pointer);
    }
  }

private:
  // Storage space used for handler-based custom memory allocation.
  typename std::aligned_storage<1024>::type storage_;

  // Whether the handler-based custom allocation storage has been used.
  bool in_use_;
};

// Wrapper class template for handler objects to allow handler memory
// allocation to be customised. Calls to operator() are forwarded to the
// encapsulated handler.
template <typename Handler>
class custom_alloc_handler
{
public:
  custom_alloc_handler(handler_allocator& a, Handler h)
    : allocator_(a),
      handler_(h)
  {
  }

  template <typename ...Args>
  void operator()(Args&&... args)
  {
    handler_(std::forward<Args>(args)...);
  }

  friend void* asio_handler_allocate(std::size_t size,
      custom_alloc_handler<Handler>* this_handler)
  {
    return this_handler->allocator_.allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
      custom_alloc_handler<Handler>* this_handler)
  {
    this_handler->allocator_.deallocate(pointer);
  }

private:
  handler_allocator& allocator_;
  Handler handler_;
};

// Helper function to wrap a handler object to add custom allocation.
template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
    handler_allocator& a, Handler h)
{
  return custom_alloc_handler<Handler>(a, h);
}

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_),
        make_custom_alloc_handler(allocator_,
          [this, self](boost::system::error_code ec, std::size_t length)
          {
            if (!ec)
            {
              do_write(length);
            }
          }));
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        make_custom_alloc_handler(allocator_,
          [this, self](boost::system::error_code ec, std::size_t /*length*/)
          {
            if (!ec)
            {
              do_read();
            }
          }));
  }

  // The socket used to communicate with the client.
  tcp::socket socket_;

  // Buffer used to store data received from the client.
  std::array<char, 1024> data_;

  // The allocator to use for handler-based custom memory allocation.
  handler_allocator allocator_;
};

class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      socket_(io_service)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket_))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: server <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;
    server s(io_service, std::atoi(argv[1]));
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
