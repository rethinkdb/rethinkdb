//
// detail/reactive_socket_recvfrom_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_REACTIVE_SOCKET_RECVFROM_OP_HPP
#define BOOST_ASIO_DETAIL_REACTIVE_SOCKET_RECVFROM_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/addressof.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/reactor_op.hpp>
#include <boost/asio/detail/socket_ops.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename MutableBufferSequence, typename Endpoint>
class reactive_socket_recvfrom_op_base : public reactor_op
{
public:
  reactive_socket_recvfrom_op_base(socket_type socket, int protocol_type,
      const MutableBufferSequence& buffers, Endpoint& endpoint,
      socket_base::message_flags flags, func_type complete_func)
    : reactor_op(&reactive_socket_recvfrom_op_base::do_perform, complete_func),
      socket_(socket),
      protocol_type_(protocol_type),
      buffers_(buffers),
      sender_endpoint_(endpoint),
      flags_(flags)
  {
  }

  static bool do_perform(reactor_op* base)
  {
    reactive_socket_recvfrom_op_base* o(
        static_cast<reactive_socket_recvfrom_op_base*>(base));

    buffer_sequence_adapter<boost::asio::mutable_buffer,
        MutableBufferSequence> bufs(o->buffers_);

    std::size_t addr_len = o->sender_endpoint_.capacity();
    bool result = socket_ops::non_blocking_recvfrom(o->socket_,
        bufs.buffers(), bufs.count(), o->flags_,
        o->sender_endpoint_.data(), &addr_len,
        o->ec_, o->bytes_transferred_);

    if (result && !o->ec_)
      o->sender_endpoint_.resize(addr_len);

    return result;
  }

private:
  socket_type socket_;
  int protocol_type_;
  MutableBufferSequence buffers_;
  Endpoint& sender_endpoint_;
  socket_base::message_flags flags_;
};

template <typename MutableBufferSequence, typename Endpoint, typename Handler>
class reactive_socket_recvfrom_op :
  public reactive_socket_recvfrom_op_base<MutableBufferSequence, Endpoint>
{
public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(reactive_socket_recvfrom_op);

  reactive_socket_recvfrom_op(socket_type socket, int protocol_type,
      const MutableBufferSequence& buffers, Endpoint& endpoint,
      socket_base::message_flags flags, Handler& handler)
    : reactive_socket_recvfrom_op_base<MutableBufferSequence, Endpoint>(
        socket, protocol_type, buffers, endpoint, flags,
        &reactive_socket_recvfrom_op::do_complete),
      handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler))
  {
  }

  static void do_complete(io_service_impl* owner, operation* base,
      const boost::system::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the handler object.
    reactive_socket_recvfrom_op* o(
        static_cast<reactive_socket_recvfrom_op*>(base));
    ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

    BOOST_ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder2<Handler, boost::system::error_code, std::size_t>
      handler(o->handler_, o->ec_, o->bytes_transferred_);
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
      boost_asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  Handler handler_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_REACTIVE_SOCKET_RECVFROM_OP_HPP
