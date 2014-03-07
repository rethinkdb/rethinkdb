//
// ssl/impl/error.ipp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_IMPL_ERROR_IPP
#define BOOST_ASIO_SSL_IMPL_ERROR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/detail/openssl_init.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace error {

namespace detail {

class ssl_category : public boost::system::error_category
{
public:
  const char* name() const BOOST_ASIO_ERROR_CATEGORY_NOEXCEPT
  {
    return "asio.ssl";
  }

  std::string message(int value) const
  {
    const char* s = ::ERR_reason_error_string(value);
    return s ? s : "asio.ssl error";
  }
};

} // namespace detail

const boost::system::error_category& get_ssl_category()
{
  static detail::ssl_category instance;
  return instance;
}

} // namespace error
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_IMPL_ERROR_IPP
