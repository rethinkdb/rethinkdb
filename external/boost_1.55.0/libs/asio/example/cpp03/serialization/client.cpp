//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <vector>
#include "connection.hpp" // Must come before boost/serialization headers.
#include <boost/serialization/vector.hpp>
#include "stock.hpp"

namespace s11n_example {

/// Downloads stock quote information from a server.
class client
{
public:
  /// Constructor starts the asynchronous connect operation.
  client(boost::asio::io_service& io_service,
      const std::string& host, const std::string& service)
    : connection_(io_service)
  {
    // Resolve the host name into an IP address.
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(host, service);
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator =
      resolver.resolve(query);

    // Start an asynchronous connect operation.
    boost::asio::async_connect(connection_.socket(), endpoint_iterator,
        boost::bind(&client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  /// Handle completion of a connect operation.
  void handle_connect(const boost::system::error_code& e)
  {
    if (!e)
    {
      // Successfully established connection. Start operation to read the list
      // of stocks. The connection::async_read() function will automatically
      // decode the data that is read from the underlying socket.
      connection_.async_read(stocks_,
          boost::bind(&client::handle_read, this,
            boost::asio::placeholders::error));
    }
    else
    {
      // An error occurred. Log it and return. Since we are not starting a new
      // operation the io_service will run out of work to do and the client will
      // exit.
      std::cerr << e.message() << std::endl;
    }
  }

  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& e)
  {
    if (!e)
    {
      // Print out the data that was received.
      for (std::size_t i = 0; i < stocks_.size(); ++i)
      {
        std::cout << "Stock number " << i << "\n";
        std::cout << "  code: " << stocks_[i].code << "\n";
        std::cout << "  name: " << stocks_[i].name << "\n";
        std::cout << "  open_price: " << stocks_[i].open_price << "\n";
        std::cout << "  high_price: " << stocks_[i].high_price << "\n";
        std::cout << "  low_price: " << stocks_[i].low_price << "\n";
        std::cout << "  last_price: " << stocks_[i].last_price << "\n";
        std::cout << "  buy_price: " << stocks_[i].buy_price << "\n";
        std::cout << "  buy_quantity: " << stocks_[i].buy_quantity << "\n";
        std::cout << "  sell_price: " << stocks_[i].sell_price << "\n";
        std::cout << "  sell_quantity: " << stocks_[i].sell_quantity << "\n";
      }
    }
    else
    {
      // An error occurred.
      std::cerr << e.message() << std::endl;
    }

    // Since we are not starting a new operation the io_service will run out of
    // work to do and the client will exit.
  }

private:
  /// The connection to the server.
  connection connection_;

  /// The data received from the server.
  std::vector<stock> stocks_;
};

} // namespace s11n_example

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;
    s11n_example::client client(io_service, argv[1], argv[2]);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
