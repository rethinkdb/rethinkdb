//
// ssl/impl/context.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2005 Voipster / Indrek dot Juhani at voipster dot com
// Copyright (c) 2005-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_IMPL_CONTEXT_HPP
#define BOOST_ASIO_SSL_IMPL_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
# include <boost/asio/detail/throw_error.hpp>
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ssl {

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)

template <typename VerifyCallback>
void context::set_verify_callback(VerifyCallback callback)
{
  boost::system::error_code ec;
  this->set_verify_callback(callback, ec);
  boost::asio::detail::throw_error(ec, "set_verify_callback");
}

template <typename VerifyCallback>
boost::system::error_code context::set_verify_callback(
    VerifyCallback callback, boost::system::error_code& ec)
{
  return do_set_verify_callback(
      new detail::verify_callback<VerifyCallback>(callback), ec);
}

template <typename PasswordCallback>
void context::set_password_callback(PasswordCallback callback)
{
  boost::system::error_code ec;
  this->set_password_callback(callback, ec);
  boost::asio::detail::throw_error(ec, "set_password_callback");
}

template <typename PasswordCallback>
boost::system::error_code context::set_password_callback(
    PasswordCallback callback, boost::system::error_code& ec)
{
  return do_set_password_callback(
      new detail::password_callback<PasswordCallback>(callback), ec);
}

#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_IMPL_CONTEXT_HPP
