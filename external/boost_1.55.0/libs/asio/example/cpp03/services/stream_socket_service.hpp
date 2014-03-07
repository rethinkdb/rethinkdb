//
// stream_socket_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERVICES_STREAM_SOCKET_SERVICE_HPP
#define SERVICES_STREAM_SOCKET_SERVICE_HPP

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include "logger.hpp"

namespace services {

/// Debugging stream socket service that wraps the normal stream socket service.
template <typename Protocol>
class stream_socket_service
  : public boost::asio::io_service::service
{
private:
  /// The type of the wrapped stream socket service.
  typedef boost::asio::stream_socket_service<Protocol> service_impl_type;

public:
  /// The unique service identifier.
  static boost::asio::io_service::id id;

  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  /// The implementation type of a stream socket.
  typedef typename service_impl_type::implementation_type implementation_type;

  /// The native type of a stream socket.
  typedef typename service_impl_type::native_handle_type native_handle_type;

  /// Construct a new stream socket service for the specified io_service.
  explicit stream_socket_service(boost::asio::io_service& io_service)
    : boost::asio::io_service::service(io_service),
      service_impl_(boost::asio::use_service<service_impl_type>(io_service)),
      logger_(io_service, "stream_socket")
  {
  }

  /// Destroy all user-defined handler objects owned by the service.
  void shutdown_service()
  {
  }

  /// Construct a new stream socket implementation.
  void construct(implementation_type& impl)
  {
    service_impl_.construct(impl);
  }

  /// Destroy a stream socket implementation.
  void destroy(implementation_type& impl)
  {
    service_impl_.destroy(impl);
  }

  /// Open a new stream socket implementation.
  boost::system::error_code open(implementation_type& impl,
      const protocol_type& protocol, boost::system::error_code& ec)
  {
    logger_.log("Opening new socket");
    return service_impl_.open(impl, protocol, ec);
  }

  /// Open a stream socket from an existing native socket.
  boost::system::error_code assign(implementation_type& impl,
      const protocol_type& protocol, const native_handle_type& native_socket,
      boost::system::error_code& ec)
  {
    logger_.log("Assigning from a native socket");
    return service_impl_.assign(impl, protocol, native_socket, ec);
  }

  /// Determine whether the socket is open.
  bool is_open(const implementation_type& impl) const
  {
    logger_.log("Checking if socket is open");
    return service_impl_.is_open(impl);
  }

  /// Close a stream socket implementation.
  boost::system::error_code close(implementation_type& impl,
      boost::system::error_code& ec)
  {
    logger_.log("Closing socket");
    return service_impl_.close(impl, ec);
  }

  /// Determine whether the socket is at the out-of-band data mark.
  bool at_mark(const implementation_type& impl,
      boost::system::error_code& ec) const
  {
    logger_.log("Checking if socket is at out-of-band data mark");
    return service_impl_.at_mark(impl, ec);
  }

  /// Determine the number of bytes available for reading.
  std::size_t available(const implementation_type& impl,
      boost::system::error_code& ec) const
  {
    logger_.log("Determining number of bytes available for reading");
    return service_impl_.available(impl, ec);
  }

  /// Bind the stream socket to the specified local endpoint.
  boost::system::error_code bind(implementation_type& impl,
      const endpoint_type& endpoint, boost::system::error_code& ec)
  {
    logger_.log("Binding socket");
    return service_impl_.bind(impl, endpoint, ec);
  }

  /// Connect the stream socket to the specified endpoint.
  boost::system::error_code connect(implementation_type& impl,
      const endpoint_type& peer_endpoint, boost::system::error_code& ec)
  {
    logger_.log("Connecting socket to " +
        boost::lexical_cast<std::string>(peer_endpoint));
    return service_impl_.connect(impl, peer_endpoint, ec);
  }

  /// Handler to wrap asynchronous connect completion.
  template <typename Handler>
  class connect_handler
  {
  public:
    connect_handler(Handler h, logger& l)
      : handler_(h),
        logger_(l)
    {
    }

    void operator()(const boost::system::error_code& e)
    {
      if (e)
      {
        std::string msg = "Asynchronous connect failed: ";
        msg += e.message();
        logger_.log(msg);
      }
      else
      {
        logger_.log("Asynchronous connect succeeded");
      }

      handler_(e);
    }

  private:
    Handler handler_;
    logger& logger_;
  };

  /// Start an asynchronous connect.
  template <typename Handler>
  void async_connect(implementation_type& impl,
      const endpoint_type& peer_endpoint, Handler handler)
  {
    logger_.log("Starting asynchronous connect to " +
        boost::lexical_cast<std::string>(peer_endpoint));
    service_impl_.async_connect(impl, peer_endpoint, 
        connect_handler<Handler>(handler, logger_));
  }

  /// Set a socket option.
  template <typename Option>
  boost::system::error_code set_option(implementation_type& impl,
      const Option& option, boost::system::error_code& ec)
  {
    logger_.log("Setting socket option");
    return service_impl_.set_option(impl, option, ec);
  }

  /// Get a socket option.
  template <typename Option>
  boost::system::error_code get_option(const implementation_type& impl,
      Option& option, boost::system::error_code& ec) const
  {
    logger_.log("Getting socket option");
    return service_impl_.get_option(impl, option, ec);
  }

  /// Perform an IO control command on the socket.
  template <typename IO_Control_Command>
  boost::system::error_code io_control(implementation_type& impl,
      IO_Control_Command& command, boost::system::error_code& ec)
  {
    logger_.log("Performing IO control command on socket");
    return service_impl_.io_control(impl, command, ec);
  }

  /// Get the local endpoint.
  endpoint_type local_endpoint(const implementation_type& impl,
      boost::system::error_code& ec) const
  {
    logger_.log("Getting socket's local endpoint");
    return service_impl_.local_endpoint(impl, ec);
  }

  /// Get the remote endpoint.
  endpoint_type remote_endpoint(const implementation_type& impl,
      boost::system::error_code& ec) const
  {
    logger_.log("Getting socket's remote endpoint");
    return service_impl_.remote_endpoint(impl, ec);
  }

  /// Disable sends or receives on the socket.
  boost::system::error_code shutdown(implementation_type& impl,
      boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec)
  {
    logger_.log("Shutting down socket");
    return service_impl_.shutdown(impl, what, ec);
  }

  /// Send the given data to the peer.
  template <typename Const_Buffers>
  std::size_t send(implementation_type& impl, const Const_Buffers& buffers,
      boost::asio::socket_base::message_flags flags,
      boost::system::error_code& ec)
  {
    logger_.log("Sending data on socket");
    return service_impl_.send(impl, buffers, flags, ec);
  }

  /// Handler to wrap asynchronous send completion.
  template <typename Handler>
  class send_handler
  {
  public:
    send_handler(Handler h, logger& l)
      : handler_(h),
        logger_(l)
    {
    }

    void operator()(const boost::system::error_code& e,
        std::size_t bytes_transferred)
    {
      if (e)
      {
        std::string msg = "Asynchronous send failed: ";
        msg += e.message();
        logger_.log(msg);
      }
      else
      {
        logger_.log("Asynchronous send succeeded");
      }

      handler_(e, bytes_transferred);
    }

  private:
    Handler handler_;
    logger& logger_;
  };

  /// Start an asynchronous send.
  template <typename Const_Buffers, typename Handler>
  void async_send(implementation_type& impl, const Const_Buffers& buffers,
      boost::asio::socket_base::message_flags flags, Handler handler)
  {
    logger_.log("Starting asynchronous send");
    service_impl_.async_send(impl, buffers, flags,
        send_handler<Handler>(handler, logger_));
  }

  /// Receive some data from the peer.
  template <typename Mutable_Buffers>
  std::size_t receive(implementation_type& impl,
      const Mutable_Buffers& buffers,
      boost::asio::socket_base::message_flags flags,
      boost::system::error_code& ec)
  {
    logger_.log("Receiving data on socket");
    return service_impl_.receive(impl, buffers, flags, ec);
  }

  /// Handler to wrap asynchronous receive completion.
  template <typename Handler>
  class receive_handler
  {
  public:
    receive_handler(Handler h, logger& l)
      : handler_(h),
        logger_(l)
    {
    }

    void operator()(const boost::system::error_code& e,
        std::size_t bytes_transferred)
    {
      if (e)
      {
        std::string msg = "Asynchronous receive failed: ";
        msg += e.message();
        logger_.log(msg);
      }
      else
      {
        logger_.log("Asynchronous receive succeeded");
      }

      handler_(e, bytes_transferred);
    }

  private:
    Handler handler_;
    logger& logger_;
  };

  /// Start an asynchronous receive.
  template <typename Mutable_Buffers, typename Handler>
  void async_receive(implementation_type& impl, const Mutable_Buffers& buffers,
      boost::asio::socket_base::message_flags flags, Handler handler)
  {
    logger_.log("Starting asynchronous receive");
    service_impl_.async_receive(impl, buffers, flags,
        receive_handler<Handler>(handler, logger_));
  }

private:
  /// The wrapped stream socket service.
  service_impl_type& service_impl_;

  /// The logger used for writing debug messages.
  mutable logger logger_;
};

template <typename Protocol>
boost::asio::io_service::id stream_socket_service<Protocol>::id;

} // namespace services

#endif // SERVICES_STREAM_SOCKET_SERVICE_HPP
