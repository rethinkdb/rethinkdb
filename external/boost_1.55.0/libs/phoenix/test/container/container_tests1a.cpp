/*=============================================================================
    Copyright (c) 2004 Angus Leeming

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include "container_tests.hpp"

std::list<int> const build_list()
{
    std::vector<int> const data = build_vector();
    return std::list<int>(data.begin(), data.end());
}

std::vector<int> const init_vector()
{
    typedef std::vector<int> int_vector;
    int const data[] = { -4, -3, -2, -1, 0 };
    int_vector::size_type const data_size = sizeof(data) / sizeof(data[0]);
    return int_vector(data, data + data_size);
}

std::vector<int> const build_vector()
{
    typedef std::vector<int> int_vector;
    static int_vector data = init_vector();
    int_vector::size_type const size = data.size();
    int_vector::iterator it = data.begin();
    int_vector::iterator const end = data.end();
    for (; it != end; ++it)
      *it += size;
    return data;
}

int 
main()
{
    std::list<int> const data = build_list();
    test_assign(data);
    test_assign2(data);
    test_back(data);
    test_begin(data);
    test_clear(data);
    return boost::report_errors();
}

