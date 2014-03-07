//  (C) Copyright Andy Tompkins 2010. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_string_generator.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/config.hpp>
#include <string>

int main(int, char*[])
{
    using namespace boost::uuids;
    
    uuid nil_uuid = {{0}};
    BOOST_TEST_EQ(nil_uuid.is_nil(), true);

    string_generator gen;
    uuid u = gen("00000000-0000-0000-0000-000000000000");
    BOOST_TEST_EQ(u, nil_uuid);
    BOOST_TEST_EQ(u.is_nil(), true);

    const uuid u_increasing = {{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef }};
    const uuid u_decreasing = {{ 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 }};

    u = gen("0123456789abcdef0123456789ABCDEF");
    BOOST_TEST_EQ(u, u_increasing);
    
    u = gen("{0123456789abcdef0123456789ABCDEF}");
    BOOST_TEST_EQ(u, u_increasing);

    u = gen("{01234567-89AB-CDEF-0123-456789abcdef}");
    BOOST_TEST_EQ(u, u_increasing);

    u = gen("01234567-89AB-CDEF-0123-456789abcdef");
    BOOST_TEST_EQ(u, u_increasing);
    
    u = gen(std::string("fedcba98-7654-3210-fedc-ba9876543210"));
    BOOST_TEST_EQ(u, u_decreasing);

#ifndef BOOST_NO_STD_WSTRING
    u = gen(L"fedcba98-7654-3210-fedc-ba9876543210");
    BOOST_TEST_EQ(u, u_decreasing);

    u = gen(std::wstring(L"01234567-89ab-cdef-0123-456789abcdef"));
    BOOST_TEST_EQ(u, u_increasing);
#endif //BOOST_NO_STD_WSTRING

    return boost::report_errors();
}
