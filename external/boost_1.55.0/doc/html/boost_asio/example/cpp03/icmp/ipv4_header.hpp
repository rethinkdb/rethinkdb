//
// ipv4_header.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef IPV4_HEADER_HPP
#define IPV4_HEADER_HPP

#include <algorithm>
#include <boost/asio/ip/address_v4.hpp>

// Packet header for IPv4.
//
// The wire format of an IPv4 header is:
// 
// 0               8               16                             31
// +-------+-------+---------------+------------------------------+      ---
// |       |       |               |                              |       ^
// |version|header |    type of    |    total length in bytes     |       |
// |  (4)  | length|    service    |                              |       |
// +-------+-------+---------------+-+-+-+------------------------+       |
// |                               | | | |                        |       |
// |        identification         |0|D|M|    fragment offset     |       |
// |                               | |F|F|                        |       |
// +---------------+---------------+-+-+-+------------------------+       |
// |               |               |                              |       |
// | time to live  |   protocol    |       header checksum        |   20 bytes
// |               |               |                              |       |
// +---------------+---------------+------------------------------+       |
// |                                                              |       |
// |                      source IPv4 address                     |       |
// |                                                              |       |
// +--------------------------------------------------------------+       |
// |                                                              |       |
// |                   destination IPv4 address                   |       |
// |                                                              |       v
// +--------------------------------------------------------------+      ---
// |                                                              |       ^
// |                                                              |       |
// /                        options (if any)                      /    0 - 40
// /                                                              /     bytes
// |                                                              |       |
// |                                                              |       v
// +--------------------------------------------------------------+      ---

class ipv4_header
{
public:
  ipv4_header() { std::fill(rep_, rep_ + sizeof(rep_), 0); }

  unsigned char version() const { return (rep_[0] >> 4) & 0xF; }
  unsigned short header_length() const { return (rep_[0] & 0xF) * 4; }
  unsigned char type_of_service() const { return rep_[1]; }
  unsigned short total_length() const { return decode(2, 3); }
  unsigned short identification() const { return decode(4, 5); }
  bool dont_fragment() const { return (rep_[6] & 0x40) != 0; }
  bool more_fragments() const { return (rep_[6] & 0x20) != 0; }
  unsigned short fragment_offset() const { return decode(6, 7) & 0x1FFF; }
  unsigned int time_to_live() const { return rep_[8]; }
  unsigned char protocol() const { return rep_[9]; }
  unsigned short header_checksum() const { return decode(10, 11); }

  boost::asio::ip::address_v4 source_address() const
  {
    boost::asio::ip::address_v4::bytes_type bytes
      = { { rep_[12], rep_[13], rep_[14], rep_[15] } };
    return boost::asio::ip::address_v4(bytes);
  }

  boost::asio::ip::address_v4 destination_address() const
  {
    boost::asio::ip::address_v4::bytes_type bytes
      = { { rep_[16], rep_[17], rep_[18], rep_[19] } };
    return boost::asio::ip::address_v4(bytes);
  }

  friend std::istream& operator>>(std::istream& is, ipv4_header& header)
  {
    is.read(reinterpret_cast<char*>(header.rep_), 20);
    if (header.version() != 4)
      is.setstate(std::ios::failbit);
    std::streamsize options_length = header.header_length() - 20;
    if (options_length < 0 || options_length > 40)
      is.setstate(std::ios::failbit);
    else
      is.read(reinterpret_cast<char*>(header.rep_) + 20, options_length);
    return is;
  }

private:
  unsigned short decode(int a, int b) const
    { return (rep_[a] << 8) + rep_[b]; }

  unsigned char rep_[60];
};

#endif // IPV4_HEADER_HPP
