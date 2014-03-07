//  (C) Copyright Andy Tompkins 2009. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_uuid_class.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/detail/lightweight_test.hpp>

class uuid_class : public boost::uuids::uuid
{
public:
    uuid_class()
        : boost::uuids::uuid(boost::uuids::random_generator()())
    {}
    
    explicit uuid_class(boost::uuids::uuid const& u)
        : boost::uuids::uuid(u)
    {}
};

int main(int, char*[])
{
    uuid_class u1;
    uuid_class u2;
    BOOST_TEST_NE(u1, u2);
    BOOST_TEST_EQ(u1.is_nil(), false);
    BOOST_TEST_EQ(u2.is_nil(), false);

    u2 = u1;
    BOOST_TEST_EQ(u1, u2);

    return boost::report_errors();
}
