//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_parser.hpp"
#include <algorithm>
#include <cctype>
#include <boost/lexical_cast.hpp>
#include "request.hpp"

namespace http {
namespace server4 {

// Enable the pseudo-keywords reenter, yield and fork.
#include <boost/asio/yield.hpp>

std::string request_parser::content_length_name_ = "Content-Length";

boost::tribool request_parser::consume(request& req, char c)
{
  reenter (this)
  {
    req.method.clear();
    req.uri.clear();
    req.http_version_major = 0;
    req.http_version_minor = 0;
    req.headers.clear();
    req.content.clear();
    content_length_ = 0;

    // Request method.
    while (is_char(c) && !is_ctl(c) && !is_tspecial(c) && c != ' ')
    {
      req.method.push_back(c);
      yield return boost::indeterminate;
    }
    if (req.method.empty())
      return false;

    // Space.
    if (c != ' ') return false;
    yield return boost::indeterminate;

    // URI.
    while (!is_ctl(c) && c != ' ')
    {
      req.uri.push_back(c);
      yield return boost::indeterminate;
    }
    if (req.uri.empty()) return false;

    // Space.
    if (c != ' ') return false;
    yield return boost::indeterminate;

    // HTTP protocol identifier.
    if (c != 'H') return false;
    yield return boost::indeterminate;
    if (c != 'T') return false;
    yield return boost::indeterminate;
    if (c != 'T') return false;
    yield return boost::indeterminate;
    if (c != 'P') return false;
    yield return boost::indeterminate;

    // Slash.
    if (c != '/') return false;
    yield return boost::indeterminate;

    // Major version number.
    if (!is_digit(c)) return false;
    while (is_digit(c))
    {
      req.http_version_major = req.http_version_major * 10 + c - '0';
      yield return boost::indeterminate;
    }

    // Dot.
    if (c != '.') return false;
    yield return boost::indeterminate;

    // Minor version number.
    if (!is_digit(c)) return false;
    while (is_digit(c))
    {
      req.http_version_minor = req.http_version_minor * 10 + c - '0';
      yield return boost::indeterminate;
    }

    // CRLF.
    if (c != '\r') return false;
    yield return boost::indeterminate;
    if (c != '\n') return false;
    yield return boost::indeterminate;

    // Headers.
    while ((is_char(c) && !is_ctl(c) && !is_tspecial(c) && c != '\r')
        || (c == ' ' || c == '\t'))
    {
      if (c == ' ' || c == '\t')
      {
        // Leading whitespace. Must be continuation of previous header's value.
        if (req.headers.empty()) return false;
        while (c == ' ' || c == '\t')
          yield return boost::indeterminate;
      }
      else
      {
        // Start the next header.
        req.headers.push_back(header());

        // Header name.
        while (is_char(c) && !is_ctl(c) && !is_tspecial(c) && c != ':')
        {
          req.headers.back().name.push_back(c);
          yield return boost::indeterminate;
        }

        // Colon and space separates the header name from the header value.
        if (c != ':') return false;
        yield return boost::indeterminate;
        if (c != ' ') return false;
        yield return boost::indeterminate;
      }

      // Header value.
      while (is_char(c) && !is_ctl(c) && c != '\r')
      {
        req.headers.back().value.push_back(c);
        yield return boost::indeterminate;
      }

      // CRLF.
      if (c != '\r') return false;
      yield return boost::indeterminate;
      if (c != '\n') return false;
      yield return boost::indeterminate;
    }

    // CRLF.
    if (c != '\r') return false;
    yield return boost::indeterminate;
    if (c != '\n') return false;

    // Check for optional Content-Length header.
    for (std::size_t i = 0; i < req.headers.size(); ++i)
    {
      if (headers_equal(req.headers[i].name, content_length_name_))
      {
        try
        {
          content_length_ =
            boost::lexical_cast<std::size_t>(req.headers[i].value);
        }
        catch (boost::bad_lexical_cast&)
        {
          return false;
        }
      }
    }

    // Content.
    while (req.content.size() < content_length_)
    {
      yield return boost::indeterminate;
      req.content.push_back(c);
    }
  }

  return true;
}

// Disable the pseudo-keywords reenter, yield and fork.
#include <boost/asio/unyield.hpp>

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

bool request_parser::tolower_compare(char a, char b)
{
  return std::tolower(a) == std::tolower(b);
}

bool request_parser::headers_equal(const std::string& a, const std::string& b)
{
  if (a.length() != b.length())
    return false;

  return std::equal(a.begin(), a.end(), b.begin(),
      &request_parser::tolower_compare);
}

} // namespace server4
} // namespace http
