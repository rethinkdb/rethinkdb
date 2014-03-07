//
// ssl/basic_context.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_BASIC_CONTEXT_HPP
#define BOOST_ASIO_SSL_BASIC_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_ENABLE_OLD_SSL)
# include <boost/asio/ssl/old/basic_context.hpp>
#endif // defined(BOOST_ASIO_ENABLE_OLD_SSL)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ssl {

#if defined(BOOST_ASIO_ENABLE_OLD_SSL)

using boost::asio::ssl::old::basic_context;

#endif // defined(BOOST_ASIO_ENABLE_OLD_SSL)

} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_BASIC_CONTEXT_HPP
