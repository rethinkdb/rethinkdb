//  Copyright (c) 2001-2012 Hartmut Kaiser
//  Copyright (c) 2012 yyyy yyyy <typhoonking77@hotmail.com>
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <string>
#include <vector>

#include<boost/spirit/include/karma.hpp>

int main()
{
    namespace karma = boost::spirit::karma;

    int num[] = {0, 1, 2, 3, 4, 5};
    std::vector<int> contents(num, num + sizeof(num) / sizeof(int));

    {
        std::string result;
        BOOST_TEST(karma::generate(std::back_inserter(result),
            *karma::center[karma::int_], contents));
        BOOST_TEST(result == "     0         1         2         3         4         5    ");
    }

    {
        std::string result;
        BOOST_TEST(karma::generate(std::back_inserter(result),
            *karma::center(5)[karma::int_], contents));
        BOOST_TEST(result == "  0    1    2    3    4    5  ");
    }

    {
        std::string result;
        BOOST_TEST(karma::generate(std::back_inserter(result),
            *karma::center("_")[karma::int_], contents));
        BOOST_TEST(result == "_____0_________1_________2_________3_________4_________5____");
    }

    {
        std::string result;
        BOOST_TEST(karma::generate(std::back_inserter(result),
            *karma::center(5, "_")[karma::int_], contents));
        BOOST_TEST(result == "__0____1____2____3____4____5__");
    }

    {
        std::string result;
        BOOST_TEST(karma::generate(std::back_inserter(result),
            *karma::center(karma::char_("_"))[karma::int_], contents));
        BOOST_TEST(result == "_____0_________1_________2_________3_________4_________5____");
    }

    {
        std::string result;
        BOOST_TEST(karma::generate(std::back_inserter(result),
            *karma::center(5, karma::char_("_"))[karma::int_], contents));
        BOOST_TEST(result == "__0____1____2____3____4____5__");
    }

    return boost::report_errors();
}

