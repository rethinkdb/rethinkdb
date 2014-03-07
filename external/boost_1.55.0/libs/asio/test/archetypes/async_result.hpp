//
// async_result.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ARCHETYPES_ASYNC_RESULT_HPP
#define ARCHETYPES_ASYNC_RESULT_HPP

#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>

namespace archetypes {

struct lazy_handler
{
};

struct concrete_handler
{
  concrete_handler(lazy_handler)
  {
  }

  template <typename Arg1>
  void operator()(Arg1)
  {
  }

  template <typename Arg1, typename Arg2>
  void operator()(Arg1, Arg2)
  {
  }
};

} // namespace archetypes

namespace boost {
namespace asio {

template <typename Signature>
struct handler_type<archetypes::lazy_handler, Signature>
{
  typedef archetypes::concrete_handler type;
};

template <>
class async_result<archetypes::concrete_handler>
{
public:
  // The return type of the initiating function.
  typedef int type;

  // Construct an async_result from a given handler.
  explicit async_result(archetypes::concrete_handler&)
  {
  }

  // Obtain the value to be returned from the initiating function.
  type get()
  {
    return 42;
  }
};

} // namespace asio
} // namespace boost

#endif // ARCHETYPES_ASYNC_RESULT_HPP
