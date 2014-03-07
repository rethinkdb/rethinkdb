//
// io_control_command.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ARCHETYPES_IO_CONTROL_COMMAND_HPP
#define ARCHETYPES_IO_CONTROL_COMMAND_HPP

namespace archetypes {

class io_control_command
{
public:
  int name() const
  {
    return 0;
  }

  void* data()
  {
    return 0;
  }
};

} // namespace archetypes

#endif // ARCHETYPES_IO_CONTROL_COMMAND_HPP
