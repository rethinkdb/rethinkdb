/*=============================================================================
    Copyright (c) 2004 Angus Leeming

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include "container_tests.hpp"

std::deque<int> const build_deque()
{
    std::vector<int> const data = build_vector();
    return std::deque<int>(data.begin(), data.end());
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
    std::deque<int> const data = build_deque();
    test_assign(data);
    test_assign2(data);
    test_at(data);
    test_back(data);
    test_begin(data);
    test_clear(data);
    test_front(data);
    test_empty(data);
    test_end(data);
    test_erase(data);
    test_get_allocator(data);
    return boost::report_errors();
}


