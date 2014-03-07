//
// read_at.cpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Test that header file is self-contained.
#include <boost/asio/read_at.hpp>

#include <cstring>
#include "archetypes/async_result.hpp"
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include "unit_test.hpp"

#if defined(BOOST_ASIO_HAS_BOOST_BIND)
# include <boost/bind.hpp>
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
# include <functional>
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

#if defined(BOOST_ASIO_HAS_BOOST_ARRAY)
#include <boost/array.hpp>
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)

#if defined(BOOST_ASIO_HAS_STD_ARRAY)
# include <array>
#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)

using namespace std; // For memcmp, memcpy and memset.

class test_random_access_device
{
public:
  typedef boost::asio::io_service io_service_type;

  test_random_access_device(boost::asio::io_service& io_service)
    : io_service_(io_service),
      length_(0),
      next_read_length_(0)
  {
  }

  io_service_type& get_io_service()
  {
    return io_service_;
  }

  void reset(const void* data, size_t length)
  {
    BOOST_ASIO_CHECK(length <= max_length);

    length_ = 0;
    while (length_ + length < max_length)
    {
      memcpy(data_ + length_, data, length);
      length_ += length;
    }

    next_read_length_ = length;
  }

  void next_read_length(size_t length)
  {
    next_read_length_ = length;
  }

  template <typename Const_Buffers>
  bool check_buffers(boost::asio::uint64_t offset,
      const Const_Buffers& buffers, size_t length)
  {
    if (offset + length > max_length)
      return false;

    typename Const_Buffers::const_iterator iter = buffers.begin();
    typename Const_Buffers::const_iterator end = buffers.end();
    size_t checked_length = 0;
    for (; iter != end && checked_length < length; ++iter)
    {
      size_t buffer_length = boost::asio::buffer_size(*iter);
      if (buffer_length > length - checked_length)
        buffer_length = length - checked_length;
      if (memcmp(data_ + offset + checked_length,
            boost::asio::buffer_cast<const void*>(*iter), buffer_length) != 0)
        return false;
      checked_length += buffer_length;
    }

    return true;
  }

  template <typename Mutable_Buffers>
  size_t read_some_at(boost::asio::uint64_t offset,
      const Mutable_Buffers& buffers)
  {
    return boost::asio::buffer_copy(buffers,
        boost::asio::buffer(data_, length_) + offset,
        next_read_length_);
  }

  template <typename Mutable_Buffers>
  size_t read_some_at(boost::asio::uint64_t offset,
      const Mutable_Buffers& buffers, boost::system::error_code& ec)
  {
    ec = boost::system::error_code();
    return read_some_at(offset, buffers);
  }

  template <typename Mutable_Buffers, typename Handler>
  void async_read_some_at(boost::asio::uint64_t offset,
      const Mutable_Buffers& buffers, Handler handler)
  {
    size_t bytes_transferred = read_some_at(offset, buffers);
    io_service_.post(boost::asio::detail::bind_handler(
          handler, boost::system::error_code(), bytes_transferred));
  }

private:
  io_service_type& io_service_;
  enum { max_length = 8192 };
  char data_[max_length];
  size_t length_;
  size_t next_read_length_;
};

static const char read_data[]
  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void test_3_arg_mutable_buffers_1_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_3_arg_vector_buffers_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::vector<boost::asio::mutable_buffer> buffers;
  buffers.push_back(boost::asio::buffer(read_buf, 32));
  buffers.push_back(boost::asio::buffer(read_buf) + 32);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_3_arg_streambuf_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  boost::asio::streambuf sb(sizeof(read_data));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  size_t bytes_transferred = boost::asio::read_at(s, 0, sb);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
}

void test_4_arg_nothrow_mutable_buffers_1_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_4_arg_nothrow_vector_buffers_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::vector<boost::asio::mutable_buffer> buffers;
  buffers.push_back(boost::asio::buffer(read_buf, 32));
  buffers.push_back(boost::asio::buffer(read_buf) + 32);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_4_arg_nothrow_streambuf_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  boost::asio::streambuf sb(sizeof(read_data));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::read_at(s, 0, sb, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);
}

bool old_style_transfer_all(const boost::system::error_code& ec,
    size_t /*bytes_transferred*/)
{
  return !!ec;
}

size_t short_transfer(const boost::system::error_code& ec,
    size_t /*bytes_transferred*/)
{
  return !!ec ? 0 : 3;
}

void test_4_arg_mutable_buffers_1_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_4_arg_vector_buffers_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::vector<boost::asio::mutable_buffer> buffers;
  buffers.push_back(boost::asio::buffer(read_buf, 32));
  buffers.push_back(boost::asio::buffer(read_buf) + 32);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_4_arg_streambuf_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  boost::asio::streambuf sb(sizeof(read_data));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  size_t bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(sb.size() == 50);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(sb.size() == 50);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 50));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
}

void test_5_arg_mutable_buffers_1_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_5_arg_vector_buffers_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::vector<boost::asio::mutable_buffer> buffers;
  buffers.push_back(boost::asio::buffer(read_buf, 32));
  buffers.push_back(boost::asio::buffer(read_buf) + 32);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, buffers,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_5_arg_streambuf_read_at()
{
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  boost::asio::streambuf sb(sizeof(read_data));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(sb.size() == 50);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 50));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(sb.size() == 50);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 50));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 0, sb,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  error = boost::system::error_code();
  bytes_transferred = boost::asio::read_at(s, 1234, sb,
      short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
  BOOST_ASIO_CHECK(!error);
}

void async_read_handler(const boost::system::error_code& e,
    size_t bytes_transferred, size_t expected_bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(!e);
  BOOST_ASIO_CHECK(bytes_transferred == expected_bytes_transferred);
}

void test_4_arg_mutable_buffers_1_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_4_arg_boost_array_buffers_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

#if defined(BOOST_ASIO_HAS_BOOST_ARRAY)
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::array<boost::asio::mutable_buffer, 2> buffers = { {
    boost::asio::buffer(read_buf, 32),
    boost::asio::buffer(read_buf) + 32 } };

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)
}

void test_4_arg_std_array_buffers_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

#if defined(BOOST_ASIO_HAS_STD_ARRAY)
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::array<boost::asio::mutable_buffer, 2> buffers = { {
    boost::asio::buffer(read_buf, 32),
    boost::asio::buffer(read_buf) + 32 } };

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)
}

void test_4_arg_vector_buffers_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::vector<boost::asio::mutable_buffer> buffers;
  buffers.push_back(boost::asio::buffer(read_buf, 32));
  buffers.push_back(boost::asio::buffer(read_buf) + 32);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_4_arg_streambuf_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_random_access_device s(ios);
  boost::asio::streambuf sb(sizeof(read_data));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bool called = false;
  boost::asio::async_read_at(s, 0, sb,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  int i = boost::asio::async_read_at(s, 1234, sb,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
}

void test_5_arg_mutable_buffers_1_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      short_transfer, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_5_arg_boost_array_buffers_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

#if defined(BOOST_ASIO_HAS_BOOST_ARRAY)
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  boost::array<boost::asio::mutable_buffer, 2> buffers = { {
    boost::asio::buffer(read_buf, 32),
    boost::asio::buffer(read_buf) + 32 } };

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      short_transfer, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)
}

void test_5_arg_std_array_buffers_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

#if defined(BOOST_ASIO_HAS_STD_ARRAY)
  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::array<boost::asio::mutable_buffer, 2> buffers = { {
    boost::asio::buffer(read_buf, 32),
    boost::asio::buffer(read_buf) + 32 } };

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      short_transfer, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)
}

void test_5_arg_vector_buffers_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_random_access_device s(ios);
  char read_buf[sizeof(read_data)];
  std::vector<boost::asio::mutable_buffer> buffers;
  buffers.push_back(boost::asio::buffer(read_buf, 32));
  buffers.push_back(boost::asio::buffer(read_buf) + 32);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 50));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 1));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, 42));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 0, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(0, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  boost::asio::async_read_at(s, 1234, buffers, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  int i = boost::asio::async_read_at(s, 1234, buffers,
      short_transfer, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, buffers, sizeof(read_data)));
}

void test_5_arg_streambuf_async_read_at()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_random_access_device s(ios);
  boost::asio::streambuf sb(sizeof(read_data));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  bool called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_all(),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(1),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 50);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 50));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_at_least(42),
      bindns::bind(async_read_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 50);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 50));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(1),
      bindns::bind(async_read_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 1);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 1));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(10),
      bindns::bind(async_read_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 10);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 10));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb,
      boost::asio::transfer_exactly(42),
      bindns::bind(async_read_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == 42);
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), 42));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb, old_style_transfer_all,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 0, sb, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(0, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  sb.consume(sb.size());
  called = false;
  boost::asio::async_read_at(s, 1234, sb, short_transfer,
      bindns::bind(async_read_handler,
        _1, _2, sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(sb.size() == sizeof(read_data));
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  sb.consume(sb.size());
  int i = boost::asio::async_read_at(s, 1234, sb,
      short_transfer, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(1234, sb.data(), sizeof(read_data)));
}

BOOST_ASIO_TEST_SUITE
(
  "read_at",
  BOOST_ASIO_TEST_CASE(test_3_arg_mutable_buffers_1_read_at)
  BOOST_ASIO_TEST_CASE(test_3_arg_vector_buffers_read_at)
  BOOST_ASIO_TEST_CASE(test_3_arg_streambuf_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_nothrow_mutable_buffers_1_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_nothrow_vector_buffers_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_nothrow_streambuf_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_mutable_buffers_1_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_vector_buffers_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_streambuf_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_mutable_buffers_1_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_vector_buffers_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_streambuf_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_mutable_buffers_1_async_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_boost_array_buffers_async_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_std_array_buffers_async_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_vector_buffers_async_read_at)
  BOOST_ASIO_TEST_CASE(test_4_arg_streambuf_async_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_mutable_buffers_1_async_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_boost_array_buffers_async_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_std_array_buffers_async_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_vector_buffers_async_read_at)
  BOOST_ASIO_TEST_CASE(test_5_arg_streambuf_async_read_at)
)
