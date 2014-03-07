//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

//----------------------------------------------------------------------

class subscriber
{
public:
  virtual ~subscriber() {}
  virtual void deliver(const std::string& msg) = 0;
};

typedef boost::shared_ptr<subscriber> subscriber_ptr;

//----------------------------------------------------------------------

class channel
{
public:
  void join(subscriber_ptr subscriber)
  {
    subscribers_.insert(subscriber);
  }

  void leave(subscriber_ptr subscriber)
  {
    subscribers_.erase(subscriber);
  }

  void deliver(const std::string& msg)
  {
    std::for_each(subscribers_.begin(), subscribers_.end(),
        boost::bind(&subscriber::deliver, _1, boost::ref(msg)));
  }

private:
  std::set<subscriber_ptr> subscribers_;
};

//----------------------------------------------------------------------

//
// This class manages socket timeouts by applying the concept of a deadline.
// Some asynchronous operations are given deadlines by which they must complete.
// Deadlines are enforced by two "actors" that persist for the lifetime of the
// session object, one for input and one for output:
//
//  +----------------+                     +----------------+
//  |                |                     |                |
//  | check_deadline |<---+                | check_deadline |<---+
//  |                |    | async_wait()   |                |    | async_wait()
//  +----------------+    |  on input      +----------------+    |  on output
//              |         |  deadline                  |         |  deadline
//              +---------+                            +---------+
//
// If either deadline actor determines that the corresponding deadline has
// expired, the socket is closed and any outstanding operations are cancelled.
//
// The input actor reads messages from the socket, where messages are delimited
// by the newline character:
//
//  +------------+
//  |            |
//  | start_read |<---+
//  |            |    |
//  +------------+    |
//          |         |
//  async_- |    +-------------+
//   read_- |    |             |
//  until() +--->| handle_read |
//               |             |
//               +-------------+
//
// The deadline for receiving a complete message is 30 seconds. If a non-empty
// message is received, it is delivered to all subscribers. If a heartbeat (a
// message that consists of a single newline character) is received, a heartbeat
// is enqueued for the client, provided there are no other messages waiting to
// be sent.
//
// The output actor is responsible for sending messages to the client:
//
//  +--------------+
//  |              |<---------------------+
//  | await_output |                      |
//  |              |<---+                 |
//  +--------------+    |                 |
//      |      |        | async_wait()    |
//      |      +--------+                 |
//      V                                 |
//  +-------------+               +--------------+
//  |             | async_write() |              |
//  | start_write |-------------->| handle_write |
//  |             |               |              |
//  +-------------+               +--------------+
//
// The output actor first waits for an output message to be enqueued. It does
// this by using a deadline_timer as an asynchronous condition variable. The
// deadline_timer will be signalled whenever the output queue is non-empty.
//
// Once a message is available, it is sent to the client. The deadline for
// sending a complete message is 30 seconds. After the message is successfully
// sent, the output actor again waits for the output queue to become non-empty.
//
class tcp_session
  : public subscriber,
    public boost::enable_shared_from_this<tcp_session>
{
public:
  tcp_session(boost::asio::io_service& io_service, channel& ch)
    : channel_(ch),
      socket_(io_service),
      input_deadline_(io_service),
      non_empty_output_queue_(io_service),
      output_deadline_(io_service)
  {
    input_deadline_.expires_at(boost::posix_time::pos_infin);
    output_deadline_.expires_at(boost::posix_time::pos_infin);

    // The non_empty_output_queue_ deadline_timer is set to pos_infin whenever
    // the output queue is empty. This ensures that the output actor stays
    // asleep until a message is put into the queue.
    non_empty_output_queue_.expires_at(boost::posix_time::pos_infin);
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  // Called by the server object to initiate the four actors.
  void start()
  {
    channel_.join(shared_from_this());

    start_read();

    input_deadline_.async_wait(
        boost::bind(&tcp_session::check_deadline,
        shared_from_this(), &input_deadline_));

    await_output();

    output_deadline_.async_wait(
        boost::bind(&tcp_session::check_deadline,
        shared_from_this(), &output_deadline_));
  }

private:
  void stop()
  {
    channel_.leave(shared_from_this());

    boost::system::error_code ignored_ec;
    socket_.close(ignored_ec);
    input_deadline_.cancel();
    non_empty_output_queue_.cancel();
    output_deadline_.cancel();
  }

  bool stopped() const
  {
    return !socket_.is_open();
  }

  void deliver(const std::string& msg)
  {
    output_queue_.push_back(msg + "\n");

    // Signal that the output queue contains messages. Modifying the expiry
    // will wake the output actor, if it is waiting on the timer.
    non_empty_output_queue_.expires_at(boost::posix_time::neg_infin);
  }

  void start_read()
  {
    // Set a deadline for the read operation.
    input_deadline_.expires_from_now(boost::posix_time::seconds(30));

    // Start an asynchronous operation to read a newline-delimited message.
    boost::asio::async_read_until(socket_, input_buffer_, '\n',
        boost::bind(&tcp_session::handle_read, shared_from_this(), _1));
  }

  void handle_read(const boost::system::error_code& ec)
  {
    if (stopped())
      return;

    if (!ec)
    {
      // Extract the newline-delimited message from the buffer.
      std::string msg;
      std::istream is(&input_buffer_);
      std::getline(is, msg);

      if (!msg.empty())
      {
        channel_.deliver(msg);
      }
      else
      {
        // We received a heartbeat message from the client. If there's nothing
        // else being sent or ready to be sent, send a heartbeat right back.
        if (output_queue_.empty())
        {
          output_queue_.push_back("\n");

          // Signal that the output queue contains messages. Modifying the
          // expiry will wake the output actor, if it is waiting on the timer.
          non_empty_output_queue_.expires_at(boost::posix_time::neg_infin);
        }
      }

      start_read();
    }
    else
    {
      stop();
    }
  }

  void await_output()
  {
    if (stopped())
      return;

    if (output_queue_.empty())
    {
      // There are no messages that are ready to be sent. The actor goes to
      // sleep by waiting on the non_empty_output_queue_ timer. When a new
      // message is added, the timer will be modified and the actor will wake.
      non_empty_output_queue_.expires_at(boost::posix_time::pos_infin);
      non_empty_output_queue_.async_wait(
          boost::bind(&tcp_session::await_output, shared_from_this()));
    }
    else
    {
      start_write();
    }
  }

  void start_write()
  {
    // Set a deadline for the write operation.
    output_deadline_.expires_from_now(boost::posix_time::seconds(30));

    // Start an asynchronous operation to send a message.
    boost::asio::async_write(socket_,
        boost::asio::buffer(output_queue_.front()),
        boost::bind(&tcp_session::handle_write, shared_from_this(), _1));
  }

  void handle_write(const boost::system::error_code& ec)
  {
    if (stopped())
      return;

    if (!ec)
    {
      output_queue_.pop_front();

      await_output();
    }
    else
    {
      stop();
    }
  }

  void check_deadline(deadline_timer* deadline)
  {
    if (stopped())
      return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline->expires_at() <= deadline_timer::traits_type::now())
    {
      // The deadline has passed. Stop the session. The other actors will
      // terminate as soon as possible.
      stop();
    }
    else
    {
      // Put the actor back to sleep.
      deadline->async_wait(
          boost::bind(&tcp_session::check_deadline,
          shared_from_this(), deadline));
    }
  }

  channel& channel_;
  tcp::socket socket_;
  boost::asio::streambuf input_buffer_;
  deadline_timer input_deadline_;
  std::deque<std::string> output_queue_;
  deadline_timer non_empty_output_queue_;
  deadline_timer output_deadline_;
};

typedef boost::shared_ptr<tcp_session> tcp_session_ptr;

//----------------------------------------------------------------------

class udp_broadcaster
  : public subscriber
{
public:
  udp_broadcaster(boost::asio::io_service& io_service,
      const udp::endpoint& broadcast_endpoint)
    : socket_(io_service)
  {
    socket_.connect(broadcast_endpoint);
  }

private:
  void deliver(const std::string& msg)
  {
    boost::system::error_code ignored_ec;
    socket_.send(boost::asio::buffer(msg), 0, ignored_ec);
  }

  udp::socket socket_;
};

//----------------------------------------------------------------------

class server
{
public:
  server(boost::asio::io_service& io_service,
      const tcp::endpoint& listen_endpoint,
      const udp::endpoint& broadcast_endpoint)
    : io_service_(io_service),
      acceptor_(io_service, listen_endpoint)
  {
    subscriber_ptr bc(new udp_broadcaster(io_service_, broadcast_endpoint));
    channel_.join(bc);

    start_accept();
  }

  void start_accept()
  {
    tcp_session_ptr new_session(new tcp_session(io_service_, channel_));

    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session, _1));
  }

  void handle_accept(tcp_session_ptr session,
      const boost::system::error_code& ec)
  {
    if (!ec)
    {
      session->start();
    }

    start_accept();
  }

private:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  channel channel_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    using namespace std; // For atoi.

    if (argc != 4)
    {
      std::cerr << "Usage: server <listen_port> <bcast_address> <bcast_port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::endpoint listen_endpoint(tcp::v4(), atoi(argv[1]));

    udp::endpoint broadcast_endpoint(
        boost::asio::ip::address::from_string(argv[2]), atoi(argv[3]));

    server s(io_service, listen_endpoint, broadcast_endpoint);

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
