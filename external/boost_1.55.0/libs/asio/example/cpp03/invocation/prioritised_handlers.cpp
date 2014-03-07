//
// prioritised_handlers.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <iostream>
#include <queue>

using boost::asio::ip::tcp;

class handler_priority_queue
{
public:
  void add(int priority, boost::function<void()> function)
  {
    handlers_.push(queued_handler(priority, function));
  }

  void execute_all()
  {
    while (!handlers_.empty())
    {
      queued_handler handler = handlers_.top();
      handler.execute();
      handlers_.pop();
    }
  }

  // A generic wrapper class for handlers to allow the invocation to be hooked.
  template <typename Handler>
  class wrapped_handler
  {
  public:
    wrapped_handler(handler_priority_queue& q, int p, Handler h)
      : queue_(q), priority_(p), handler_(h)
    {
    }

    void operator()()
    {
      handler_();
    }

    template <typename Arg1>
    void operator()(Arg1 arg1)
    {
      handler_(arg1);
    }

    template <typename Arg1, typename Arg2>
    void operator()(Arg1 arg1, Arg2 arg2)
    {
      handler_(arg1, arg2);
    }

  //private:
    handler_priority_queue& queue_;
    int priority_;
    Handler handler_;
  };

  template <typename Handler>
  wrapped_handler<Handler> wrap(int priority, Handler handler)
  {
    return wrapped_handler<Handler>(*this, priority, handler);
  }

private:
  class queued_handler
  {
  public:
    queued_handler(int p, boost::function<void()> f)
      : priority_(p), function_(f)
    {
    }

    void execute()
    {
      function_();
    }

    friend bool operator<(const queued_handler& a,
        const queued_handler& b)
    {
      return a.priority_ < b.priority_;
    }

  private:
    int priority_;
    boost::function<void()> function_;
  };

  std::priority_queue<queued_handler> handlers_;
};

// Custom invocation hook for wrapped handlers.
template <typename Function, typename Handler>
void asio_handler_invoke(Function f,
    handler_priority_queue::wrapped_handler<Handler>* h)
{
  h->queue_.add(h->priority_, f);
}

//----------------------------------------------------------------------

void high_priority_handler(const boost::system::error_code& /*ec*/)
{
  std::cout << "High priority handler\n";
}

void middle_priority_handler(const boost::system::error_code& /*ec*/)
{
  std::cout << "Middle priority handler\n";
}

void low_priority_handler()
{
  std::cout << "Low priority handler\n";
}

int main()
{
  boost::asio::io_service io_service;

  handler_priority_queue pri_queue;

  // Post a completion handler to be run immediately.
  io_service.post(pri_queue.wrap(0, low_priority_handler));

  // Start an asynchronous accept that will complete immediately.
  tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 0);
  tcp::acceptor acceptor(io_service, endpoint);
  tcp::socket server_socket(io_service);
  acceptor.async_accept(server_socket,
      pri_queue.wrap(100, high_priority_handler));
  tcp::socket client_socket(io_service);
  client_socket.connect(acceptor.local_endpoint());

  // Set a deadline timer to expire immediately.
  boost::asio::deadline_timer timer(io_service);
  timer.expires_at(boost::posix_time::neg_infin);
  timer.async_wait(pri_queue.wrap(42, middle_priority_handler));

  while (io_service.run_one())
  {
    // The custom invocation hook adds the handlers to the priority queue
    // rather than executing them from within the poll_one() call.
    while (io_service.poll_one())
      ;

    pri_queue.execute_all();
  }

  return 0;
}
