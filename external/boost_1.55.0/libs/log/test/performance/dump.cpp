/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   dump.cpp
 * \author Andrey Semashev
 * \date   05.05.2013
 *
 * \brief  This code measures performance dumping binary data
 */

#include <cstdlib>
#include <iomanip>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include <boost/cstdint.hpp>
#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/dump.hpp>

namespace logging = boost::log;

const unsigned int base_loop_count = 10000;

void test(std::size_t block_size)
{
    std::cout << "Block size: " << block_size << " bytes.";

    std::vector< boost::uint8_t > data;
    data.resize(block_size);
    std::generate_n(data.begin(), block_size, &std::rand);

    std::string str;
    logging::formatting_ostream strm(str);

    const boost::uint8_t* const p = &data[0];

    boost::uint64_t data_processed = 0, duration = 0;
    boost::posix_time::ptime start, end;
    start = boost::date_time::microsec_clock< boost::posix_time::ptime >::universal_time();
    do
    {
        for (unsigned int i = 0; i < base_loop_count; ++i)
        {
            strm << logging::dump(p, block_size);
            str.clear();
        }
        end = boost::date_time::microsec_clock< boost::posix_time::ptime >::universal_time();
        data_processed += base_loop_count * block_size;
        duration = (end - start).total_microseconds();
    }
    while (duration < 2000000);

    std::cout << " Test duration: " << duration << " us ("
        << std::fixed << std::setprecision(3) << static_cast< double >(data_processed) / (static_cast< double >(duration) * (1048576.0 / 1000000.0))
        << " MiB per second)" << std::endl;
}

int main(int argc, char* argv[])
{
    test(32);
    test(128);
    test(1024);
    test(16384);
    test(1048576);

    return 0;
}
