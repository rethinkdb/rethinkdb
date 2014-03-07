//
// file_handler.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_FILE_HANDLER_HPP
#define HTTP_SERVER4_FILE_HANDLER_HPP

#include <string>

namespace http {
namespace server4 {

struct reply;
struct request;

/// The common handler for all incoming requests.
class file_handler
{
public:
  /// Construct with a directory containing files to be served.
  explicit file_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void operator()(const request& req, reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_FILE_HANDLER_HPP
