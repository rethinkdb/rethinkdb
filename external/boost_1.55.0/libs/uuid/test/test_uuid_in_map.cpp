//  (C) Copyright Andy Tompkins 2011. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// libs/uuid/test/test_uuid_in_map.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <map>

#include <boost/detail/lightweight_test.hpp>

int main(int, char*[])
{
    boost::uuids::random_generator gen;

    boost::uuids::uuid u1 = gen();
    boost::uuids::uuid u2 = gen();
    boost::uuids::uuid u3 = gen();

    std::map<boost::uuids::uuid, int> uuid_map;
    uuid_map[u1] = 1;
    uuid_map[u2] = 2;
    uuid_map[u3] = 3;

    BOOST_TEST_EQ(1, uuid_map[u1]);
    BOOST_TEST_EQ(2, uuid_map[u2]);
    BOOST_TEST_EQ(3, uuid_map[u3]);

    return boost::report_errors();
}
