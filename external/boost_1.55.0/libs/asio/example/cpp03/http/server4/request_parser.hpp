//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_REQUEST_PARSER_HPP
#define HTTP_SERVER4_REQUEST_PARSER_HPP

#include <string>
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/asio/coroutine.hpp>

namespace http {
namespace server4 {

struct request;

/// Parser for incoming requests.
class request_parser : boost::asio::coroutine
{
public:
  /// Parse some data. The tribool return value is true when a complete request
  /// has been parsed, false if the data is invalid, indeterminate when more
  /// data is required. The InputIterator return value indicates how much of the
  /// input has been consumed.
  template <typename InputIterator>
  boost::tuple<boost::tribool, InputIterator> parse(request& req,
      InputIterator begin, InputIterator end)
  {
    while (begin != end)
    {
      boost::tribool result = consume(req, *begin++);
      if (result || !result)
        return boost::make_tuple(result, begin);
    }
    boost::tribool result = boost::indeterminate;
    return boost::make_tuple(result, begin);
  }

private:
  /// The name of the content length header.
  static std::string content_length_name_;

  /// Content length as decoded from headers. Defaults to 0.
  std::size_t content_length_;

  /// Handle the next character of input.
  boost::tribool consume(request& req, char input);

  /// Check if a byte is an HTTP character.
  static bool is_char(int c);

  /// Check if a byte is an HTTP control character.
  static bool is_ctl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
  static bool is_tspecial(int c);

  /// Check if a byte is a digit.
  static bool is_digit(int c);

  /// Check if two characters are equal, without regard to case.
  static bool tolower_compare(char a, char b);

  /// Check whether the two request header names match.
  bool headers_equal(const std::string& a, const std::string& b);
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_REQUEST_PARSER_HPP
