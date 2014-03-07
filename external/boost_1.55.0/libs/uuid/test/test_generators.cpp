//  (C) Copyright Andy Tompkins 2009. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_generators.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/test/test_tools.hpp>

#include <boost/random.hpp>

template <typename RandomUuidGenerator>
void check_random_generator(RandomUuidGenerator& uuid_gen)
{
    boost::uuids::uuid u1 = uuid_gen();
    boost::uuids::uuid u2 = uuid_gen();

    BOOST_CHECK_NE(u1, u2);

    // check variant
    BOOST_CHECK_EQUAL(u1.variant(), boost::uuids::uuid::variant_rfc_4122);
//    BOOST_CHECK_EQUAL( *(u1.begin()+8) & 0xC0, 0x80);
    // version
    BOOST_CHECK_EQUAL( *(u1.begin()+6) & 0xF0, 0x40);
}

int test_main(int, char*[])
{
    using namespace boost::uuids;
    using boost::test_tools::output_test_stream;

    { // test nil generator
        uuid u1 = nil_generator()();
        uuid u2 = {{0}};
        BOOST_CHECK_EQUAL(u1, u2);

        uuid u3 = nil_uuid();
        BOOST_CHECK_EQUAL(u3, u2);
    }
    
    { // test string_generator
        string_generator gen;
        uuid u = gen("00000000-0000-0000-0000-000000000000");
        BOOST_CHECK_EQUAL(u, nil_uuid());
        BOOST_CHECK_EQUAL(u.is_nil(), true);

        const uuid u_increasing = {{ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef }};
        const uuid u_decreasing = {{ 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 }};

        u = gen("0123456789abcdef0123456789ABCDEF");
        BOOST_CHECK_EQUAL(u, u_increasing);
        
        u = gen("{0123456789abcdef0123456789ABCDEF}");
        BOOST_CHECK_EQUAL(u, u_increasing);

        u = gen("{01234567-89AB-CDEF-0123-456789abcdef}");
        BOOST_CHECK_EQUAL(u, u_increasing);

        u = gen("01234567-89AB-CDEF-0123-456789abcdef");
        BOOST_CHECK_EQUAL(u, u_increasing);
        
        u = gen(std::string("fedcba98-7654-3210-fedc-ba9876543210"));
        BOOST_CHECK_EQUAL(u, u_decreasing);

#ifndef BOOST_NO_STD_WSTRING
        u = gen(L"fedcba98-7654-3210-fedc-ba9876543210");
        BOOST_CHECK_EQUAL(u, u_decreasing);

        u = gen(std::wstring(L"01234567-89ab-cdef-0123-456789abcdef"));
        BOOST_CHECK_EQUAL(u, u_increasing);
#endif //BOOST_NO_STD_WSTRING
    }
    
    { // test name_generator
        uuid dns_namespace_uuid = {{0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}};
        uuid correct = {{0x21, 0xf7, 0xf8, 0xde, 0x80, 0x51, 0x5b, 0x89, 0x86, 0x80, 0x01, 0x95, 0xef, 0x79, 0x8b, 0x6a}};
        uuid wcorrect = {{0xb9, 0x50, 0x66, 0x13, 0x2c, 0x04, 0x51, 0x2d, 0xb8, 0xfe, 0xbf, 0x8d, 0x0b, 0xa1, 0xb2, 0x71}};

        name_generator gen(dns_namespace_uuid);

        uuid u = gen("www.widgets.com");
        BOOST_CHECK_EQUAL(u, correct);
        BOOST_CHECK_EQUAL(u.variant(), boost::uuids::uuid::variant_rfc_4122);

        u = gen(L"www.widgets.com");
        BOOST_CHECK_EQUAL(u, wcorrect);
        BOOST_CHECK_EQUAL(u.variant(), boost::uuids::uuid::variant_rfc_4122);

        u = gen(std::string("www.widgets.com"));
        BOOST_CHECK_EQUAL(u, correct);
        BOOST_CHECK_EQUAL(u.variant(), boost::uuids::uuid::variant_rfc_4122);

        u = gen(std::wstring(L"www.widgets.com"));
        BOOST_CHECK_EQUAL(u, wcorrect);
        BOOST_CHECK_EQUAL(u.variant(), boost::uuids::uuid::variant_rfc_4122);

        char name[] = "www.widgets.com";
        u = gen(name, 15);
        BOOST_CHECK_EQUAL(u, correct);
        BOOST_CHECK_EQUAL(u.variant(), boost::uuids::uuid::variant_rfc_4122);
    }

    { // test random_generator
        // default random number generator
        random_generator uuid_gen1;
        check_random_generator(uuid_gen1);
        
        // specific random number generator
        basic_random_generator<boost::rand48> uuid_gen2;
        check_random_generator(uuid_gen2);
        
        // pass by reference
        boost::ecuyer1988 ecuyer1988_gen;
        basic_random_generator<boost::ecuyer1988> uuid_gen3(ecuyer1988_gen);
        check_random_generator(uuid_gen3);
        
        // pass by pointer
        boost::lagged_fibonacci607 lagged_fibonacci607_gen;
        basic_random_generator<boost::lagged_fibonacci607> uuid_gen4(&lagged_fibonacci607_gen);
        check_random_generator(uuid_gen4);

        // random device
        //basic_random_generator<boost::random_device> uuid_gen5;
        //check_random_generator(uuid_gen5);
    }
    
    return 0;
}
