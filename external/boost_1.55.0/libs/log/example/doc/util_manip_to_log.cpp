/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/to_log.hpp>

namespace logging = boost::log;

//[ example_utility_manipulators_to_log
std::ostream& operator<<
(
    std::ostream& strm,
    logging::to_log_manip< int > const& manip
)
{
    strm << std::setw(4) << std::setfill('0') << std::hex << manip.get() << std::dec;
    return strm;
}

void test_manip()
{
    std::cout << "Regular output: " << 1010 << std::endl;
    std::cout << "Log output: " << logging::to_log(1010) << std::endl;
}
//]

//[ example_utility_manipulators_to_log_with_tag
struct tag_A;
struct tag_B;

std::ostream& operator<<
(
    std::ostream& strm,
    logging::to_log_manip< int, tag_A > const& manip
)
{
    strm << "A[" << manip.get() << "]";
    return strm;
}

std::ostream& operator<<
(
    std::ostream& strm,
    logging::to_log_manip< int, tag_B > const& manip
)
{
    strm << "B[" << manip.get() << "]";
    return strm;
}

void test_manip_with_tag()
{
    std::cout << "Regular output: " << 1010 << std::endl;
    std::cout << "Log output A: " << logging::to_log< tag_A >(1010) << std::endl;
    std::cout << "Log output B: " << logging::to_log< tag_B >(1010) << std::endl;
}
//]


int main(int, char*[])
{
    test_manip();
    test_manip_with_tag();

    return 0;
}
