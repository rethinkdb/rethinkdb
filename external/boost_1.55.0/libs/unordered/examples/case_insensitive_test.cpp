
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./case_insensitive.hpp"
#include <boost/detail/lightweight_test.hpp>
#include <boost/unordered_map.hpp>

struct word_info {
    int tag;
    explicit word_info(int t = 0) : tag(t) {}
};

void test1() {
    boost::unordered_map<std::string, word_info,
        hash_examples::ihash, hash_examples::iequal_to> idictionary;

    BOOST_TEST(idictionary.empty());

    idictionary["one"] = word_info(1);
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find("ONE") != idictionary.end() &&
        idictionary.find("ONE") == idictionary.find("one"));

    idictionary.insert(std::make_pair("ONE", word_info(2)));
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find("ONE") != idictionary.end() &&
            idictionary.find("ONE")->first == "one" &&
            idictionary.find("ONE")->second.tag == 1);

    idictionary["One"] = word_info(3);
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find("ONE") != idictionary.end() &&
            idictionary.find("ONE")->first == "one" &&
            idictionary.find("ONE")->second.tag == 3);

    idictionary["two"] = word_info(4);
    BOOST_TEST(idictionary.size() == 2);
    BOOST_TEST(idictionary.find("two") != idictionary.end() &&
            idictionary.find("TWO")->first == "two" &&
            idictionary.find("Two")->second.tag == 4);


}

void test2() {
    boost::unordered_map<std::wstring, word_info,
        hash_examples::ihash, hash_examples::iequal_to> idictionary;

    BOOST_TEST(idictionary.empty());

    idictionary[L"one"] = word_info(1);
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find(L"ONE") != idictionary.end() &&
        idictionary.find(L"ONE") == idictionary.find(L"one"));

    idictionary.insert(std::make_pair(L"ONE", word_info(2)));
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find(L"ONE") != idictionary.end() &&
            idictionary.find(L"ONE")->first == L"one" &&
            idictionary.find(L"ONE")->second.tag == 1);

    idictionary[L"One"] = word_info(3);
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find(L"ONE") != idictionary.end() &&
            idictionary.find(L"ONE")->first == L"one" &&
            idictionary.find(L"ONE")->second.tag == 3);

    idictionary[L"two"] = word_info(4);
    BOOST_TEST(idictionary.size() == 2);
    BOOST_TEST(idictionary.find(L"two") != idictionary.end() &&
            idictionary.find(L"TWO")->first == L"two" &&
            idictionary.find(L"Two")->second.tag == 4);
}

int main() {
    test1();
    test2();

    return boost::report_errors();
}
