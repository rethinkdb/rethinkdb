/*=============================================================================
    Copyright (c) 2004 Angus Leeming

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include "container_tests.hpp"
#include <boost/static_assert.hpp>

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
    std::vector<int> const data = build_vector();
    test_get_allocator(data);
    test_insert(data);
    test_max_size(data);
    test_pop_back(data);
    test_push_back(data);
    test_rbegin(data);
    test_rend(data);
    test_reserve(data);
    test_resize(data);
    test_size(data);
    return boost::report_errors();
}

