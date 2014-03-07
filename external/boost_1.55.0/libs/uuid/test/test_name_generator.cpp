//  (C) Copyright Andy Tompkins 2010. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_name_generator.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/name_generator.hpp>
#include <boost/detail/lightweight_test.hpp>

int main(int, char*[])
{
    using namespace boost::uuids;

    uuid dns_namespace_uuid = {{0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}};
    uuid correct = {{0x21, 0xf7, 0xf8, 0xde, 0x80, 0x51, 0x5b, 0x89, 0x86, 0x80, 0x01, 0x95, 0xef, 0x79, 0x8b, 0x6a}};
    uuid wcorrect = {{0xc3, 0x15, 0x27, 0x0b, 0xa4, 0x66, 0x58, 0x72, 0xac, 0xa4, 0x96, 0x26, 0xce, 0xc0, 0xf4, 0xbe}};

    name_generator gen(dns_namespace_uuid);

    uuid u = gen("www.widgets.com");
    BOOST_TEST_EQ(u, correct);
    BOOST_TEST_EQ(u.variant(), boost::uuids::uuid::variant_rfc_4122);

    u = gen(L"www.widgets.com");
    BOOST_TEST_EQ(u, wcorrect);
    BOOST_TEST_EQ(u.variant(), boost::uuids::uuid::variant_rfc_4122);

    u = gen(std::string("www.widgets.com"));
    BOOST_TEST_EQ(u, correct);
    BOOST_TEST_EQ(u.variant(), boost::uuids::uuid::variant_rfc_4122);

    u = gen(std::wstring(L"www.widgets.com"));
    BOOST_TEST_EQ(u, wcorrect);
    BOOST_TEST_EQ(u.variant(), boost::uuids::uuid::variant_rfc_4122);

    char name[] = "www.widgets.com";
    u = gen(name, 15);
    BOOST_TEST_EQ(u, correct);
    BOOST_TEST_EQ(u.variant(), boost::uuids::uuid::variant_rfc_4122);

    return boost::report_errors();
}
