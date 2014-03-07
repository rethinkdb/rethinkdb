
// Copyright 2012 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif

#include <boost/detail/lightweight_test.hpp>
#include "./compile_time.hpp"

namespace test {
    enum enum_override { enum_override1, enum_override2 };
    std::size_t hash_value(enum_override) { return 896532; }

    enum enum1 { enum1a };
    enum enum2 { enum2a, enum2b };
    enum enum3 { enum3a = 574, enum3b };
    enum enum4 { enum4a = -12574, enum4b };
}

int main() {
    compile_time_tests((test::enum1*) 0);
    compile_time_tests((test::enum2*) 0);
    compile_time_tests((test::enum3*) 0);
    compile_time_tests((test::enum4*) 0);
    compile_time_tests((test::enum_override*) 0);

    BOOST_HASH_TEST_NAMESPACE::hash<test::enum1> hash1;
    BOOST_HASH_TEST_NAMESPACE::hash<test::enum2> hash2;
    BOOST_HASH_TEST_NAMESPACE::hash<test::enum3> hash3;
    BOOST_HASH_TEST_NAMESPACE::hash<test::enum4> hash4;

    BOOST_TEST(hash1(test::enum1a) == hash1(test::enum1a));

    BOOST_TEST(hash2(test::enum2a) == hash2(test::enum2a));
    BOOST_TEST(hash2(test::enum2a) != hash2(test::enum2b));
    BOOST_TEST(hash2(test::enum2b) == hash2(test::enum2b));

    BOOST_TEST(hash3(test::enum3a) == hash3(test::enum3a));
    BOOST_TEST(hash3(test::enum3a) != hash3(test::enum3b));
    BOOST_TEST(hash3(test::enum3b) == hash3(test::enum3b));

    BOOST_TEST(hash4(test::enum4a) == hash4(test::enum4a));
    BOOST_TEST(hash4(test::enum4a) != hash4(test::enum4b));
    BOOST_TEST(hash4(test::enum4b) == hash4(test::enum4b));

    BOOST_HASH_TEST_NAMESPACE::hash<test::enum_override> hash_override;

    BOOST_TEST(hash_override(test::enum_override1) ==
        hash_override(test::enum_override1));
    BOOST_TEST(hash_override(test::enum_override1) ==
        hash_override(test::enum_override2));
    BOOST_TEST(hash_override(test::enum_override1) ==
        hash_override(test::enum_override1));

    return boost::report_errors();
}
