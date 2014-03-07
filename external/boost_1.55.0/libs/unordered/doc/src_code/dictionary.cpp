
// Copyright 2006-2007 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/unordered_map.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "../../examples/fnv1.hpp"

//[case_insensitive_functions
    struct iequal_to
        : std::binary_function<std::string, std::string, bool>
    {
        bool operator()(std::string const& x,
            std::string const& y) const
        {
            return boost::algorithm::iequals(x, y, std::locale());
        }
    };

    struct ihash
        : std::unary_function<std::string, std::size_t>
    {
        std::size_t operator()(std::string const& x) const
        {
            std::size_t seed = 0;
            std::locale locale;

            for(std::string::const_iterator it = x.begin();
                it != x.end(); ++it)
            {
                boost::hash_combine(seed, std::toupper(*it, locale));
            }

            return seed;
        }
    };
//]

int main() {
//[case_sensitive_dictionary_fnv
    boost::unordered_map<std::string, int, hash::fnv_1>
        dictionary;
//]

    BOOST_TEST(dictionary.empty());

    dictionary["one"] = 1;
    BOOST_TEST(dictionary.size() == 1);
    BOOST_TEST(dictionary.find("ONE") == dictionary.end());

    dictionary.insert(std::make_pair("ONE", 2));
    BOOST_TEST(dictionary.size() == 2);
    BOOST_TEST(dictionary.find("ONE") != dictionary.end() &&
            dictionary.find("ONE")->first == "ONE" &&
            dictionary.find("ONE")->second == 2);

    dictionary["One"] = 3;
    BOOST_TEST(dictionary.size() == 3);
    BOOST_TEST(dictionary.find("One") != dictionary.end() &&
            dictionary.find("One")->first == "One" &&
            dictionary.find("One")->second == 3);

    dictionary["two"] = 4;
    BOOST_TEST(dictionary.size() == 4);
    BOOST_TEST(dictionary.find("Two") == dictionary.end() &&
            dictionary.find("two") != dictionary.end() &&
            dictionary.find("two")->second == 4);


//[case_insensitive_dictionary
    boost::unordered_map<std::string, int, ihash, iequal_to>
        idictionary;
//]

    BOOST_TEST(idictionary.empty());

    idictionary["one"] = 1;
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find("ONE") != idictionary.end() &&
        idictionary.find("ONE") == idictionary.find("one"));

    idictionary.insert(std::make_pair("ONE", 2));
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find("ONE") != idictionary.end() &&
            idictionary.find("ONE")->first == "one" &&
            idictionary.find("ONE")->second == 1);

    idictionary["One"] = 3;
    BOOST_TEST(idictionary.size() == 1);
    BOOST_TEST(idictionary.find("ONE") != idictionary.end() &&
            idictionary.find("ONE")->first == "one" &&
            idictionary.find("ONE")->second == 3);

    idictionary["two"] = 4;
    BOOST_TEST(idictionary.size() == 2);
    BOOST_TEST(idictionary.find("two") != idictionary.end() &&
            idictionary.find("TWO")->first == "two" &&
            idictionary.find("Two")->second == 4);

    return boost::report_errors();
}
