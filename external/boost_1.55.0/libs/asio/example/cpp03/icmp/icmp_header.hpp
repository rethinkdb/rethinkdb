//
// icmp_header.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ICMP_HEADER_HPP
#define ICMP_HEADER_HPP

#include <istream>
#include <ostream>
#include <algorithm>

// ICMP header for both IPv4 and IPv6.
//
// The wire format of an ICMP header is:
// 
// 0               8               16                             31
// +---------------+---------------+------------------------------+      ---
// |               |               |                              |       ^
// |     type      |     code      |          checksum            |       |
// |               |               |                              |       |
// +---------------+---------------+------------------------------+    8 bytes
// |                               |                              |       |
// |          identifier           |       sequence number        |       |
// |                               |                              |       v
// +-------------------------------+------------------------------+      ---

class icmp_header
{
public:
  enum { echo_reply = 0, destination_unreachable = 3, source_quench = 4,
    redirect = 5, echo_request = 8, time_exceeded = 11, parameter_problem = 12,
    timestamp_request = 13, timestamp_reply = 14, info_request = 15,
    info_reply = 16, address_request = 17, address_reply = 18 };

  icmp_header() { std::fill(rep_, rep_ + sizeof(rep_), 0); }

  unsigned char type() const { return rep_[0]; }
  unsigned char code() const { return rep_[1]; }
  unsigned short checksum() const { return decode(2, 3); }
  unsigned short identifier() const { return decode(4, 5); }
  unsigned short sequence_number() const { return decode(6, 7); }

  void type(unsigned char n) { rep_[0] = n; }
  void code(unsigned char n) { rep_[1] = n; }
  void checksum(unsigned short n) { encode(2, 3, n); }
  void identifier(unsigned short n) { encode(4, 5, n); }
  void sequence_number(unsigned short n) { encode(6, 7, n); }

  friend std::istream& operator>>(std::istream& is, icmp_header& header)
    { return is.read(reinterpret_cast<char*>(header.rep_), 8); }

  friend std::ostream& operator<<(std::ostream& os, const icmp_header& header)
    { return os.write(reinterpret_cast<const char*>(header.rep_), 8); }

private:
  unsigned short decode(int a, int b) const
    { return (rep_[a] << 8) + rep_[b]; }

  void encode(int a, int b, unsigned short n)
  {
    rep_[a] = static_cast<unsigned char>(n >> 8);
    rep_[b] = static_cast<unsigned char>(n & 0xFF);
  }

  unsigned char rep_[8];
};

template <typename Iterator>
void compute_checksum(icmp_header& header,
    Iterator body_begin, Iterator body_end)
{
  unsigned int sum = (header.type() << 8) + header.code()
    + header.identifier() + header.sequence_number();

  Iterator body_iter = body_begin;
  while (body_iter != body_end)
  {
    sum += (static_cast<unsigned char>(*body_iter++) << 8);
    if (body_iter != body_end)
      sum += static_cast<unsigned char>(*body_iter++);
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  header.checksum(static_cast<unsigned short>(~sum));
}

#endif // ICMP_HEADER_HPP
