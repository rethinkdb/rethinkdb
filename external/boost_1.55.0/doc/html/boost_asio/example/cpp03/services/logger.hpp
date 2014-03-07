//
// logger.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERVICES_LOGGER_HPP
#define SERVICES_LOGGER_HPP

#include "basic_logger.hpp"
#include "logger_service.hpp"

namespace services {

/// Typedef for typical logger usage.
typedef basic_logger<logger_service> logger;

} // namespace services

#endif // SERVICES_LOGGER_HPP
