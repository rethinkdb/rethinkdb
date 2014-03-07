//  (C) Copyright Andy Tompkins 2010. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_nil_generator.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/detail/lightweight_test.hpp>

int main(int, char*[])
{
    using namespace boost::uuids;

    uuid u1 = nil_generator()();
    uuid u2 = {{0}};
    BOOST_TEST_EQ(u1, u2);

    uuid u3 = nil_uuid();
    BOOST_TEST_EQ(u3, u2);

    return boost::report_errors();
}
