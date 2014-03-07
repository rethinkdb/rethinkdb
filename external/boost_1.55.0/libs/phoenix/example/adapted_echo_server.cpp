//
// Copyright (c) 2011 Thomas Heller
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>

#define BOOST_PHOENIX_NO_PREDEFINED_TERMINALS

#include <boost/phoenix.hpp>
#include <boost/asio.hpp>

namespace phx = boost::phoenix;

using boost::phoenix::ref;

BOOST_PHOENIX_ADAPT_FUNCTION(void, read, boost::asio::async_read, 4)
BOOST_PHOENIX_ADAPT_FUNCTION(void, write, boost::asio::async_write, 3)
BOOST_PHOENIX_ADAPT_FUNCTION(boost::asio::mutable_buffers_1, buffer, boost::asio::buffer, 2)

template <typename Acceptor, typename Socket, typename Handler>
void accept_impl(Acceptor & acceptor, Socket & socket, Handler const & handler)
{
    acceptor.async_accept(socket, handler);
}
BOOST_PHOENIX_ADAPT_FUNCTION(void, accept, accept_impl, 3)

typedef phx::expression::local_variable<struct action_key>::type action;

#include <boost/function.hpp>

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    phx::lambda_type lambda;
    phx::arg_names::_1_type _1;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), std::atoi(argv[1])));
    boost::asio::ip::tcp::socket socket(io_service);
    std::size_t const max_length = 1024;
    char buf[max_length];

    std::cout << "server starting...\n";

    boost::function<void(boost::system::error_code const &)> accept_handler;
    phx::expression::argument<1>::type _error;
    phx::expression::argument<2>::type _length;
    action _action;
    BOOST_AUTO(
        create_handler
      , (lambda(_action = lambda[_1])
        [
            if_(!_error)
            [
                bind(_action, ref(socket), ref(buf), _error, _length)
            ]
            .else_
            [
                bind(&boost::asio::ip::tcp::socket::close, ref(socket))
              , accept(ref(acceptor), ref(socket), phx::ref(accept_handler))
            ]
        ])
    );
    boost::function<void(boost::system::error_code const &, std::size_t)> read_handler;
    boost::function<void(boost::system::error_code const &, std::size_t)> write_handler;

    accept_handler =
        if_(!_error)
        [
            read(ref(socket), buffer(ref(buf), max_length), boost::asio::transfer_at_least(1), phx::ref(read_handler))
        ];

    {
        phx::expression::argument<1>::type _socket;
        phx::expression::argument<2>::type _buf;
        phx::expression::argument<3>::type _error;
        phx::expression::argument<4>::type _length;
        read_handler = create_handler(
            write(_socket, buffer(_buf, _length), phx::ref(write_handler))
        );

        write_handler = create_handler(
            read(_socket, buffer(_buf, max_length), boost::asio::transfer_at_least(1), phx::ref(read_handler))
        );
    }

    acceptor.async_accept(
        socket
      , accept_handler
    );

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
