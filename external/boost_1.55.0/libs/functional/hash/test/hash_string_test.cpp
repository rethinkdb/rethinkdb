
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif

#include <boost/detail/lightweight_test.hpp>
#include <string>
#include "./compile_time.hpp"

void string_tests()
{
    compile_time_tests((std::string*) 0);

    BOOST_HASH_TEST_NAMESPACE::hash<std::string> x1;
    BOOST_HASH_TEST_NAMESPACE::hash<std::string> x2;

    BOOST_TEST(x1("Hello") == x2(std::string("Hel") + "lo"));
    BOOST_TEST(x1("") == x2(std::string()));

#if defined(BOOST_HASH_TEST_EXTENSIONS)
    std::string value1;
    std::string value2("Hello");

    BOOST_TEST(x1(value1) == BOOST_HASH_TEST_NAMESPACE::hash_value(value1));
    BOOST_TEST(x1(value2) == BOOST_HASH_TEST_NAMESPACE::hash_value(value2));
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(value1) ==
            BOOST_HASH_TEST_NAMESPACE::hash_range(value1.begin(), value1.end()));
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(value2) ==
            BOOST_HASH_TEST_NAMESPACE::hash_range(value2.begin(), value2.end()));
#endif
}

#if !defined(BOOST_NO_STD_WSTRING)
void wstring_tests()
{
    compile_time_tests((std::wstring*) 0);

    BOOST_HASH_TEST_NAMESPACE::hash<std::wstring> x1;
    BOOST_HASH_TEST_NAMESPACE::hash<std::wstring> x2;

    BOOST_TEST(x1(L"Hello") == x2(std::wstring(L"Hel") + L"lo"));
    BOOST_TEST(x1(L"") == x2(std::wstring()));

#if defined(BOOST_HASH_TEST_EXTENSIONS)
    std::wstring value1;
    std::wstring value2(L"Hello");

    BOOST_TEST(x1(value1) == BOOST_HASH_TEST_NAMESPACE::hash_value(value1));
    BOOST_TEST(x1(value2) == BOOST_HASH_TEST_NAMESPACE::hash_value(value2));
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(value1) ==
            BOOST_HASH_TEST_NAMESPACE::hash_range(value1.begin(), value1.end()));
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(value2) ==
            BOOST_HASH_TEST_NAMESPACE::hash_range(value2.begin(), value2.end()));
#endif
}
#endif

int main()
{
    string_tests();
#if !defined(BOOST_NO_STD_WSTRING)
    wstring_tests();
#endif
    return boost::report_errors();
}
