/*=============================================================================
    Copyright (c) 2004 Angus Leeming

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include "container_tests.hpp"
#include <boost/static_assert.hpp>

std::map<int, int> const build_map()
{
    typedef std::map<int, int> int_map;
    typedef std::vector<int> int_vector;

    int_map result;
    int_vector const data = build_vector();
    int_vector::const_iterator it = data.begin();
    int_vector::const_iterator const end = data.end();
    for (; it != end; ++it) {
      int const value = *it;
      result[value] = 100 * value;
    }
    return result;
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
    BOOST_STATIC_ASSERT((phx::stl::has_mapped_type<std::map<int, int> >::value));

    std::map<int, int> const data = build_map();
    test_begin(data);
    test_clear(data);
    test_empty(data);
    test_end(data);
    test_map_erase(data);
    test_get_allocator(data);
    return boost::report_errors();
}

