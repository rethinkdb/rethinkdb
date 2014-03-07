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
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include "protocol.hpp"

using namespace boost;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }
    using namespace std; // For atoi.
    std::string host_name = argv[1];
    std::string port = argv[2];

    boost::asio::io_service io_service;

    // Determine the location of the server.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(host_name, port);
    tcp::endpoint remote_endpoint = *resolver.resolve(query);

    // Establish the control connection to the server.
    tcp::socket control_socket(io_service);
    control_socket.connect(remote_endpoint);

    // Create a datagram socket to receive data from the server.
    boost::shared_ptr<udp::socket> data_socket(
        new udp::socket(io_service, udp::endpoint(udp::v4(), 0)));

    // Determine what port we will receive data on.
    udp::endpoint data_endpoint = data_socket->local_endpoint();

    // Ask the server to start sending us data.
    control_request start = control_request::start(data_endpoint.port());
    boost::asio::write(control_socket, start.to_buffers());

    unsigned long last_frame_number = 0;
    for (;;)
    {
      // Receive 50 messages on the current data socket.
      for (int i = 0; i < 50; ++i)
      {
        // Receive a frame from the server.
        frame f;
        data_socket->receive(f.to_buffers(), 0);
        if (f.number() > last_frame_number)
        {
          last_frame_number = f.number();
          std::cout << "\n" << f.payload();
        }
      }

      // Time to switch to a new socket. To ensure seamless handover we will
      // continue to receive packets using the old socket until data arrives on
      // the new one.
      std::cout << " Starting renegotiation";

      // Create the new data socket.
      boost::shared_ptr<udp::socket> new_data_socket(
          new udp::socket(io_service, udp::endpoint(udp::v4(), 0)));

      // Determine the new port we will use to receive data.
      udp::endpoint new_data_endpoint = new_data_socket->local_endpoint();

      // Ask the server to switch over to the new port.
      control_request change = control_request::change(
          data_endpoint.port(), new_data_endpoint.port());
      boost::system::error_code control_result;
      boost::asio::async_write(control_socket, change.to_buffers(),
          (
            lambda::var(control_result) = lambda::_1
          ));

      // Try to receive a frame from the server on the new data socket. If we
      // successfully receive a frame on this new data socket we can consider
      // the renegotation complete. In that case we will close the old data
      // socket, which will cause any outstanding receive operation on it to be
      // cancelled.
      frame f1;
      boost::system::error_code new_data_socket_result;
      new_data_socket->async_receive(f1.to_buffers(),
          (
            // Note: lambda::_1 is the first argument to the callback handler,
            // which in this case is the error code for the operation.
            lambda::var(new_data_socket_result) = lambda::_1,
            lambda::if_(!lambda::_1)
            [
              // We have successfully received a frame on the new data socket,
              // so we can close the old data socket. This will cancel any
              // outstanding receive operation on the old data socket.
              lambda::var(data_socket) = boost::shared_ptr<udp::socket>()
            ]
          ));

      // This loop will continue until we have successfully completed the
      // renegotiation (i.e. received a frame on the new data socket), or some
      // unrecoverable error occurs.
      bool done = false;
      while (!done)
      {
        // Even though we're performing a renegotation, we want to continue
        // receiving data as smoothly as possible. Therefore we will continue to
        // try to receive a frame from the server on the old data socket. If we
        // receive a frame on this socket we will interrupt the io_service,
        // print the frame, and resume waiting for the other operations to
        // complete.
        frame f2;
        done = true; // Let's be optimistic.
        if (data_socket) // Might have been closed by new_data_socket's handler.
        {
          data_socket->async_receive(f2.to_buffers(), 0,
              (
                lambda::if_(!lambda::_1)
                [
                  // We have successfully received a frame on the old data
                  // socket. Stop the io_service so that we can print it.
                  lambda::bind(&boost::asio::io_service::stop, &io_service),
                  lambda::var(done) = false
                ]
              ));
        }

        // Run the operations in parallel. This will block until all operations
        // have finished, or until the io_service is interrupted. (No threads!)
        io_service.reset();
        io_service.run();

        // If the io_service.run() was interrupted then we have received a frame
        // on the old data socket. We need to keep waiting for the renegotation
        // operations to complete.
        if (!done)
        {
          if (f2.number() > last_frame_number)
          {
            last_frame_number = f2.number();
            std::cout << "\n" << f2.payload();
          }
        }
      }

      // Since the loop has finished, we have either successfully completed
      // the renegotation, or an error has occurred. First we'll check for
      // errors.
      if (control_result)
        throw boost::system::system_error(control_result);
      if (new_data_socket_result)
        throw boost::system::system_error(new_data_socket_result);

      // If we get here it means we have successfully started receiving data on
      // the new data socket. This new data socket will be used from now on
      // (until the next time we renegotiate).
      std::cout << " Renegotiation complete";
      data_socket = new_data_socket;
      data_endpoint = new_data_endpoint;
      if (f1.number() > last_frame_number)
      {
        last_frame_number = f1.number();
        std::cout << "\n" << f1.payload();
      }
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
