//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>
#include "connection.hpp" // Must come before boost/serialization headers.
#include <boost/serialization/vector.hpp>
#include "stock.hpp"

namespace s11n_example {

/// Serves stock quote information to any client that connects to it.
class server
{
public:
  /// Constructor opens the acceptor and starts waiting for the first incoming
  /// connection.
  server(boost::asio::io_service& io_service, unsigned short port)
    : acceptor_(io_service,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
  {
    // Create the data to be sent to each client.
    stock s;
    s.code = "ABC";
    s.name = "A Big Company";
    s.open_price = 4.56;
    s.high_price = 5.12;
    s.low_price = 4.33;
    s.last_price = 4.98;
    s.buy_price = 4.96;
    s.buy_quantity = 1000;
    s.sell_price = 4.99;
    s.sell_quantity = 2000;
    stocks_.push_back(s);
    s.code = "DEF";
    s.name = "Developer Entertainment Firm";
    s.open_price = 20.24;
    s.high_price = 22.88;
    s.low_price = 19.50;
    s.last_price = 19.76;
    s.buy_price = 19.72;
    s.buy_quantity = 34000;
    s.sell_price = 19.85;
    s.sell_quantity = 45000;
    stocks_.push_back(s);

    // Start an accept operation for a new connection.
    connection_ptr new_conn(new connection(acceptor_.get_io_service()));
    acceptor_.async_accept(new_conn->socket(),
        boost::bind(&server::handle_accept, this,
          boost::asio::placeholders::error, new_conn));
  }

  /// Handle completion of a accept operation.
  void handle_accept(const boost::system::error_code& e, connection_ptr conn)
  {
    if (!e)
    {
      // Successfully accepted a new connection. Send the list of stocks to the
      // client. The connection::async_write() function will automatically
      // serialize the data structure for us.
      conn->async_write(stocks_,
          boost::bind(&server::handle_write, this,
            boost::asio::placeholders::error, conn));
    }

    // Start an accept operation for a new connection.
    connection_ptr new_conn(new connection(acceptor_.get_io_service()));
    acceptor_.async_accept(new_conn->socket(),
        boost::bind(&server::handle_accept, this,
          boost::asio::placeholders::error, new_conn));
  }

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& e, connection_ptr conn)
  {
    // Nothing to do. The socket will be closed automatically when the last
    // reference to the connection object goes away.
  }

private:
  /// The acceptor object used to accept incoming socket connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The data to be sent to each client.
  std::vector<stock> stocks_;
};

} // namespace s11n_example

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 2)
    {
      std::cerr << "Usage: server <port>" << std::endl;
      return 1;
    }
    unsigned short port = boost::lexical_cast<unsigned short>(argv[1]);

    boost::asio::io_service io_service;
    s11n_example::server server(io_service, port);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
