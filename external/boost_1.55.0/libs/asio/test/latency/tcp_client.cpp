//
// tcp_client.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "high_res_clock.hpp"

using boost::asio::ip::tcp;
using boost::posix_time::ptime;
using boost::posix_time::microsec_clock;

const int num_samples = 100000;

struct transfer_all
{
  typedef std::size_t result_type;
  std::size_t operator()(const boost::system::error_code& ec, std::size_t)
  {
    return (ec && ec != boost::asio::error::would_block) ? 0 : ~0;
  }
};

int main(int argc, char* argv[])
{
  if (argc != 6)
  {
    std::fprintf(stderr,
        "Usage: tcp_client <ip> <port> "
        "<nconns> <bufsize> {spin|block}\n");
    return 1;
  }

  const char* ip = argv[1];
  unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));
  int num_connections = std::atoi(argv[3]);
  std::size_t buf_size = static_cast<std::size_t>(std::atoi(argv[4]));
  bool spin = (std::strcmp(argv[5], "spin") == 0);

  boost::asio::io_service io_service;
  std::vector<boost::shared_ptr<tcp::socket> > sockets;

  for (int i = 0; i < num_connections; ++i)
  {
    boost::shared_ptr<tcp::socket> s(new tcp::socket(io_service));

    tcp::endpoint target(boost::asio::ip::address::from_string(ip), port);
    s->connect(target);

    s->set_option(tcp::no_delay(true));

    if (spin)
    {
      tcp::socket::non_blocking_io nbio(true);
      s->io_control(nbio);
    }

    sockets.push_back(s);
  }

  std::vector<unsigned char> write_buf(buf_size);
  std::vector<unsigned char> read_buf(buf_size);

  ptime start = microsec_clock::universal_time();
  boost::uint64_t start_hr = high_res_clock();

  boost::uint64_t samples[num_samples];
  for (int i = 0; i < num_samples; ++i)
  {
    tcp::socket& socket = *sockets[i % num_connections];

    boost::uint64_t t = high_res_clock();

    boost::system::error_code ec;
    boost::asio::write(socket,
        boost::asio::buffer(write_buf),
        transfer_all(), ec);

    boost::asio::read(socket,
        boost::asio::buffer(read_buf),
        transfer_all(), ec);

    samples[i] = high_res_clock() - t;
  }

  ptime stop = microsec_clock::universal_time();
  boost::uint64_t stop_hr = high_res_clock();
  boost::uint64_t elapsed_usec = (stop - start).total_microseconds();
  boost::uint64_t elapsed_hr = stop_hr - start_hr;
  double scale = 1.0 * elapsed_usec / elapsed_hr;

  std::sort(samples, samples + num_samples);
  std::printf("  0.0%%\t%f\n", samples[0] * scale);
  std::printf("  0.1%%\t%f\n", samples[num_samples / 1000 - 1] * scale);
  std::printf("  1.0%%\t%f\n", samples[num_samples / 100 - 1] * scale);
  std::printf(" 10.0%%\t%f\n", samples[num_samples / 10 - 1] * scale);
  std::printf(" 20.0%%\t%f\n", samples[num_samples * 2 / 10 - 1] * scale);
  std::printf(" 30.0%%\t%f\n", samples[num_samples * 3 / 10 - 1] * scale);
  std::printf(" 40.0%%\t%f\n", samples[num_samples * 4 / 10 - 1] * scale);
  std::printf(" 50.0%%\t%f\n", samples[num_samples * 5 / 10 - 1] * scale);
  std::printf(" 60.0%%\t%f\n", samples[num_samples * 6 / 10 - 1] * scale);
  std::printf(" 70.0%%\t%f\n", samples[num_samples * 7 / 10 - 1] * scale);
  std::printf(" 80.0%%\t%f\n", samples[num_samples * 8 / 10 - 1] * scale);
  std::printf(" 90.0%%\t%f\n", samples[num_samples * 9 / 10 - 1] * scale);
  std::printf(" 99.0%%\t%f\n", samples[num_samples * 99 / 100 - 1] * scale);
  std::printf(" 99.9%%\t%f\n", samples[num_samples * 999 / 1000 - 1] * scale);
  std::printf("100.0%%\t%f\n", samples[num_samples - 1] * scale);

  double total = 0.0;
  for (int i = 0; i < num_samples; ++i) total += samples[i] * scale;
  std::printf("  mean\t%f\n", total / num_samples);
}
