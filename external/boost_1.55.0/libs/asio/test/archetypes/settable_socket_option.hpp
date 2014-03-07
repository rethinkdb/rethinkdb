//
// settable_socket_option.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ARCHETYPES_SETTABLE_SOCKET_OPTION_HPP
#define ARCHETYPES_SETTABLE_SOCKET_OPTION_HPP

#include <cstddef>

namespace archetypes {

template <typename PointerType>
class settable_socket_option
{
public:
  template <typename Protocol>
  int level(const Protocol&) const
  {
    return 0;
  }

  template <typename Protocol>
  int name(const Protocol&) const
  {
    return 0;
  }

  template <typename Protocol>
  const PointerType* data(const Protocol&) const
  {
    return 0;
  }

  template <typename Protocol>
  std::size_t size(const Protocol&) const
  {
    return 0;
  }
};

} // namespace archetypes

#endif // ARCHETYPES_SETTABLE_SOCKET_OPTION_HPP
