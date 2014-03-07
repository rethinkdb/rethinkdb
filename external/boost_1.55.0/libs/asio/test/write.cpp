//
// write.cpp
// ~~~~~~~~~
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
#include <boost/asio/write.hpp>

#include <cstring>
#include <vector>
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

class test_stream
{
public:
  typedef boost::asio::io_service io_service_type;

  test_stream(boost::asio::io_service& io_service)
    : io_service_(io_service),
      length_(max_length),
      position_(0),
      next_write_length_(max_length)
  {
    memset(data_, 0, max_length);
  }

  io_service_type& get_io_service()
  {
    return io_service_;
  }

  void reset(size_t length = max_length)
  {
    BOOST_ASIO_CHECK(length <= max_length);

    memset(data_, 0, max_length);
    length_ = length;
    position_ = 0;
    next_write_length_ = length;
  }

  void next_write_length(size_t length)
  {
    next_write_length_ = length;
  }

  template <typename Const_Buffers>
  bool check_buffers(const Const_Buffers& buffers, size_t length)
  {
    if (length != position_)
      return false;

    typename Const_Buffers::const_iterator iter = buffers.begin();
    typename Const_Buffers::const_iterator end = buffers.end();
    size_t checked_length = 0;
    for (; iter != end && checked_length < length; ++iter)
    {
      size_t buffer_length = boost::asio::buffer_size(*iter);
      if (buffer_length > length - checked_length)
        buffer_length = length - checked_length;
      if (memcmp(data_ + checked_length,
            boost::asio::buffer_cast<const void*>(*iter), buffer_length) != 0)
        return false;
      checked_length += buffer_length;
    }

    return true;
  }

  template <typename Const_Buffers>
  size_t write_some(const Const_Buffers& buffers)
  {
    size_t n = boost::asio::buffer_copy(
        boost::asio::buffer(data_, length_) + position_,
        buffers, next_write_length_);
    position_ += n;
    return n;
  }

  template <typename Const_Buffers>
  size_t write_some(const Const_Buffers& buffers, boost::system::error_code& ec)
  {
    ec = boost::system::error_code();
    return write_some(buffers);
  }

  template <typename Const_Buffers, typename Handler>
  void async_write_some(const Const_Buffers& buffers, Handler handler)
  {
    size_t bytes_transferred = write_some(buffers);
    io_service_.post(boost::asio::detail::bind_handler(
          handler, boost::system::error_code(), bytes_transferred));
  }

private:
  io_service_type& io_service_;
  enum { max_length = 8192 };
  char data_[max_length];
  size_t length_;
  size_t position_;
  size_t next_write_length_;
};

static const char write_data[]
  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static char mutable_write_data[]
  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void test_2_arg_zero_buffers_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;

  size_t bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == 0);
}

void test_2_arg_const_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  size_t bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_2_arg_mutable_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(mutable_write_data, sizeof(mutable_write_data));

  s.reset();
  size_t bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
}

void test_2_arg_vector_buffers_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(write_data, 32));
  buffers.push_back(boost::asio::buffer(write_data) + 32);

  s.reset();
  size_t bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_3_arg_nothrow_zero_buffers_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;

  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == 0);
  BOOST_ASIO_CHECK(!error);
}

void test_3_arg_nothrow_const_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_3_arg_nothrow_mutable_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(mutable_write_data, sizeof(mutable_write_data));

  s.reset();
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_3_arg_nothrow_vector_buffers_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(write_data, 32));
  buffers.push_back(boost::asio::buffer(write_data) + 32);

  s.reset();
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
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

void test_3_arg_const_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  size_t bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_3_arg_mutable_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(mutable_write_data, sizeof(mutable_write_data));

  s.reset();
  size_t bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
}

void test_3_arg_vector_buffers_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(write_data, 32));
  buffers.push_back(boost::asio::buffer(write_data) + 32);

  s.reset();
  size_t bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all());
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42));
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1));
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10));
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42));
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, old_style_transfer_all);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  bytes_transferred = boost::asio::write(s, buffers, short_transfer);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_4_arg_const_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_4_arg_mutable_buffers_1_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(mutable_write_data, sizeof(mutable_write_data));

  s.reset();
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(mutable_write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
  BOOST_ASIO_CHECK(!error);
}

void test_4_arg_vector_buffers_write()
{
  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(write_data, 32));
  buffers.push_back(boost::asio::buffer(write_data) + 32);

  s.reset();
  boost::system::error_code error;
  size_t bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_all(), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_at_least(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 50);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(1), error);
  BOOST_ASIO_CHECK(bytes_transferred == 1);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(10), error);
  BOOST_ASIO_CHECK(bytes_transferred == 10);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      boost::asio::transfer_exactly(42), error);
  BOOST_ASIO_CHECK(bytes_transferred == 42);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers,
      old_style_transfer_all, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(1);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);

  s.reset();
  s.next_write_length(10);
  error = boost::system::error_code();
  bytes_transferred = boost::asio::write(s, buffers, short_transfer, error);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
  BOOST_ASIO_CHECK(!error);
}

void async_write_handler(const boost::system::error_code& e,
    size_t bytes_transferred, size_t expected_bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(!e);
  BOOST_ASIO_CHECK(bytes_transferred == expected_bytes_transferred);
}

void test_3_arg_const_buffers_1_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_3_arg_mutable_buffers_1_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(mutable_write_data, sizeof(mutable_write_data));

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));
}

void test_3_arg_boost_array_buffers_async_write()
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
  test_stream s(ios);
  boost::array<boost::asio::const_buffer, 2> buffers = { {
    boost::asio::buffer(write_data, 32),
    boost::asio::buffer(write_data) + 32 } };

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)
}

void test_3_arg_std_array_buffers_async_write()
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
  test_stream s(ios);
  std::array<boost::asio::const_buffer, 2> buffers = { {
    boost::asio::buffer(write_data, 32),
    boost::asio::buffer(write_data) + 32 } };

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)
}

void test_3_arg_vector_buffers_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(write_data, 32));
  buffers.push_back(boost::asio::buffer(write_data) + 32);

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_3_arg_streambuf_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::streambuf sb;
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  bool called = false;
  boost::asio::async_write(s, sb,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  int i = boost::asio::async_write(s, sb, archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_4_arg_const_buffers_1_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, short_transfer,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_4_arg_mutable_buffers_1_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::mutable_buffers_1 buffers
    = boost::asio::buffer(mutable_write_data, sizeof(mutable_write_data));

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(mutable_write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(mutable_write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, short_transfer,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_4_arg_boost_array_buffers_async_write()
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
  test_stream s(ios);
  boost::array<boost::asio::const_buffer, 2> buffers = { {
    boost::asio::buffer(write_data, 32),
    boost::asio::buffer(write_data) + 32 } };

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, short_transfer,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)
}

void test_4_arg_std_array_buffers_async_write()
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
  test_stream s(ios);
  std::array<boost::asio::const_buffer, 2> buffers = { {
    boost::asio::buffer(write_data, 32),
    boost::asio::buffer(write_data) + 32 } };

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, short_transfer,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)
}

void test_4_arg_vector_buffers_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(boost::asio::buffer(write_data, 32));
  buffers.push_back(boost::asio::buffer(write_data) + 32);

  s.reset();
  bool called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, buffers, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  int i = boost::asio::async_write(s, buffers, short_transfer,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

void test_4_arg_streambuf_async_write()
{
#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  boost::asio::io_service ios;
  test_stream s(ios);
  boost::asio::streambuf sb;
  boost::asio::const_buffers_1 buffers
    = boost::asio::buffer(write_data, sizeof(write_data));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  bool called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_all(),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(1),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_at_least(42),
      bindns::bind(async_write_handler,
        _1, _2, 50, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 50));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(1),
      bindns::bind(async_write_handler,
        _1, _2, 1, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 1));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(10),
      bindns::bind(async_write_handler,
        _1, _2, 10, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 10));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, boost::asio::transfer_exactly(42),
      bindns::bind(async_write_handler,
        _1, _2, 42, &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, 42));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, old_style_transfer_all,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  called = false;
  boost::asio::async_write(s, sb, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(1);
  called = false;
  boost::asio::async_write(s, sb, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  s.next_write_length(10);
  called = false;
  boost::asio::async_write(s, sb, short_transfer,
      bindns::bind(async_write_handler,
        _1, _2, sizeof(write_data), &called));
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(called);
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));

  s.reset();
  sb.consume(sb.size());
  sb.sputn(write_data, sizeof(write_data));
  int i = boost::asio::async_write(s, sb, short_transfer,
      archetypes::lazy_handler());
  BOOST_ASIO_CHECK(i == 42);
  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(s.check_buffers(buffers, sizeof(write_data)));
}

BOOST_ASIO_TEST_SUITE
(
  "write",
  BOOST_ASIO_TEST_CASE(test_2_arg_zero_buffers_write)
  BOOST_ASIO_TEST_CASE(test_2_arg_const_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_2_arg_mutable_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_2_arg_vector_buffers_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_nothrow_zero_buffers_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_nothrow_const_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_nothrow_mutable_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_nothrow_vector_buffers_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_const_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_mutable_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_vector_buffers_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_const_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_mutable_buffers_1_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_vector_buffers_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_const_buffers_1_async_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_mutable_buffers_1_async_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_boost_array_buffers_async_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_std_array_buffers_async_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_vector_buffers_async_write)
  BOOST_ASIO_TEST_CASE(test_3_arg_streambuf_async_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_const_buffers_1_async_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_mutable_buffers_1_async_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_boost_array_buffers_async_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_std_array_buffers_async_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_vector_buffers_async_write)
  BOOST_ASIO_TEST_CASE(test_4_arg_streambuf_async_write)
)
