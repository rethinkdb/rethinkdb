/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_attribute_set.cpp
 * \author Andrey Semashev
 * \date   24.01.2009
 *
 * \brief  This header contains tests for the attribute set class.
 */

#define BOOST_TEST_MODULE attr_attribute_set

#include <list>
#include <vector>
#include <string>
#include <utility>
#include <iterator>
#include <boost/test/unit_test.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include "char_definitions.hpp"
#include "attr_comparison.hpp"

namespace logging = boost::log;
namespace attrs = logging::attributes;

// The test checks construction and assignment
BOOST_AUTO_TEST_CASE(construction)
{
    typedef logging::attribute_set attr_set;
    typedef test_data< char > data;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1;
    BOOST_CHECK(set1.empty());
    BOOST_CHECK_EQUAL(set1.size(), 0UL);

    attr_set set2 = set1;
    BOOST_CHECK(set2.empty());
    BOOST_CHECK_EQUAL(set2.size(), 0UL);

    set2[data::attr1()] = attr1;
    set2[data::attr2()] = attr2;
    BOOST_CHECK(set1.empty());
    BOOST_CHECK_EQUAL(set1.size(), 0UL);
    BOOST_CHECK(!set2.empty());
    BOOST_CHECK_EQUAL(set2.size(), 2UL);

    attr_set set3 = set2;
    BOOST_CHECK(!set3.empty());
    BOOST_CHECK_EQUAL(set3.size(), 2UL);
    BOOST_CHECK_EQUAL(set3.count(data::attr1()), 1UL);
    BOOST_CHECK_EQUAL(set3.count(data::attr2()), 1UL);
    BOOST_CHECK_EQUAL(set3.count(data::attr3()), 0UL);

    set1[data::attr3()] = attr3;
    BOOST_CHECK(!set1.empty());
    BOOST_CHECK_EQUAL(set1.size(), 1UL);
    BOOST_CHECK_EQUAL(set1.count(data::attr3()), 1UL);

    set2 = set1;
    BOOST_REQUIRE_EQUAL(set1.size(), set2.size());
    BOOST_CHECK(std::equal(set1.begin(), set1.end(), set2.begin()));
}

// The test checks lookup methods
BOOST_AUTO_TEST_CASE(lookup)
{
    typedef logging::attribute_set attr_set;
    typedef test_data< char > data;
    typedef std::basic_string< char > string;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);

    attr_set set1;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;

    // Traditional find methods
    attr_set::iterator it = set1.find(data::attr1());
    BOOST_CHECK(it != set1.end());
    BOOST_CHECK_EQUAL(it->second, attr1);

    string s1 = data::attr2();
    it = set1.find(s1);
    BOOST_CHECK(it != set1.end());
    BOOST_CHECK_EQUAL(it->second, attr2);

    it = set1.find(data::attr1());
    BOOST_CHECK(it != set1.end());
    BOOST_CHECK_EQUAL(it->second, attr1);

    it = set1.find(data::attr3());
    BOOST_CHECK(it == set1.end());

    // Subscript operator
    logging::attribute p = set1[data::attr1()];
    BOOST_CHECK_EQUAL(p, attr1);
    BOOST_CHECK_EQUAL(set1.size(), 2UL);

    p = set1[s1];
    BOOST_CHECK_EQUAL(p, attr2);
    BOOST_CHECK_EQUAL(set1.size(), 2UL);

    p = set1[data::attr1()];
    BOOST_CHECK_EQUAL(p, attr1);
    BOOST_CHECK_EQUAL(set1.size(), 2UL);

    p = set1[data::attr3()];
    BOOST_CHECK(!p);
    BOOST_CHECK_EQUAL(set1.size(), 2UL);

    // Counting elements
    BOOST_CHECK_EQUAL(set1.count(data::attr1()), 1UL);
    BOOST_CHECK_EQUAL(set1.count(s1), 1UL);
    BOOST_CHECK_EQUAL(set1.count(data::attr1()), 1UL);
    BOOST_CHECK_EQUAL(set1.count(data::attr3()), 0UL);
}

// The test checks insertion methods
BOOST_AUTO_TEST_CASE(insertion)
{
    typedef logging::attribute_set attr_set;
    typedef test_data< char > data;
    typedef std::basic_string< char > string;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1;

    // Traditional insert methods
    std::pair< attr_set::iterator, bool > res = set1.insert(data::attr1(), attr1);
    BOOST_CHECK(res.second);
    BOOST_CHECK(res.first != set1.end());
    BOOST_CHECK(res.first->first == data::attr1());
    BOOST_CHECK_EQUAL(res.first->second, attr1);
    BOOST_CHECK(!set1.empty());
    BOOST_CHECK_EQUAL(set1.size(), 1UL);

    res = set1.insert(std::make_pair(attr_set::key_type(data::attr2()), attr2));
    BOOST_CHECK(res.second);
    BOOST_CHECK(res.first != set1.end());
    BOOST_CHECK(res.first->first == data::attr2());
    BOOST_CHECK_EQUAL(res.first->second, attr2);
    BOOST_CHECK(!set1.empty());
    BOOST_CHECK_EQUAL(set1.size(), 2UL);

    // Insertion attempt of an attribute with the name of an already existing attribute
    res = set1.insert(std::make_pair(attr_set::key_type(data::attr2()), attr3));
    BOOST_CHECK(!res.second);
    BOOST_CHECK(res.first != set1.end());
    BOOST_CHECK(res.first->first == data::attr2());
    BOOST_CHECK_EQUAL(res.first->second, attr2);
    BOOST_CHECK(!set1.empty());
    BOOST_CHECK_EQUAL(set1.size(), 2UL);

    // Mass insertion
    typedef attr_set::key_type key_type;
    std::list< std::pair< key_type, logging::attribute > > elems;
    elems.push_back(std::make_pair(key_type(data::attr2()), attr2));
    elems.push_back(std::make_pair(key_type(data::attr1()), attr1));
    elems.push_back(std::make_pair(key_type(data::attr3()), attr3));
    // ... with element duplication
    elems.push_back(std::make_pair(key_type(data::attr1()), attr3));

    attr_set set2;
    set2.insert(elems.begin(), elems.end());
    BOOST_CHECK(!set2.empty());
    BOOST_REQUIRE_EQUAL(set2.size(), 3UL);
    typedef attr_set::mapped_type mapped_type;
    BOOST_CHECK_EQUAL(static_cast< mapped_type >(set2[data::attr1()]), attr1);
    BOOST_CHECK_EQUAL(static_cast< mapped_type >(set2[data::attr2()]), attr2);
    BOOST_CHECK_EQUAL(static_cast< mapped_type >(set2[data::attr3()]), attr3);

    // The same, but with insertion results collection
    std::vector< std::pair< attr_set::iterator, bool > > results;

    attr_set set3;
    set3.insert(elems.begin(), elems.end(), std::back_inserter(results));
    BOOST_REQUIRE_EQUAL(results.size(), elems.size());
    BOOST_CHECK(!set3.empty());
    BOOST_REQUIRE_EQUAL(set3.size(), 3UL);
    attr_set::iterator it = set3.find(data::attr1());
    BOOST_REQUIRE(it != set3.end());
    BOOST_CHECK(it->first == data::attr1());
    BOOST_CHECK_EQUAL(it->second, attr1);
    BOOST_CHECK(it == results[1].first);
    it = set3.find(data::attr2());
    BOOST_REQUIRE(it != set3.end());
    BOOST_CHECK(it->first == data::attr2());
    BOOST_CHECK_EQUAL(it->second, attr2);
    BOOST_CHECK(it == results[0].first);
    it = set3.find(data::attr3());
    BOOST_REQUIRE(it != set3.end());
    BOOST_CHECK(it->first == data::attr3());
    BOOST_CHECK_EQUAL(it->second, attr3);
    BOOST_CHECK(it == results[2].first);

    BOOST_CHECK(results[0].second);
    BOOST_CHECK(results[1].second);
    BOOST_CHECK(results[2].second);
    BOOST_CHECK(!results[3].second);

    // Subscript operator
    attr_set set4;

    logging::attribute& p1 = (set4[data::attr1()] = attr1);
    BOOST_CHECK_EQUAL(set4.size(), 1UL);
    BOOST_CHECK_EQUAL(p1, attr1);

    logging::attribute& p2 = (set4[string(data::attr2())] = attr2);
    BOOST_CHECK_EQUAL(set4.size(), 2UL);
    BOOST_CHECK_EQUAL(p2, attr2);

    logging::attribute& p3 = (set4[key_type(data::attr3())] = attr3);
    BOOST_CHECK_EQUAL(set4.size(), 3UL);
    BOOST_CHECK_EQUAL(p3, attr3);

    // subscript operator can replace existing elements
    logging::attribute& p4 = (set4[data::attr3()] = attr1);
    BOOST_CHECK_EQUAL(set4.size(), 3UL);
    BOOST_CHECK_EQUAL(p4, attr1);
}

// The test checks erasure methods
BOOST_AUTO_TEST_CASE(erasure)
{
    typedef logging::attribute_set attr_set;
    typedef test_data< char > data;
    typedef std::basic_string< char > string;

    attrs::constant< int > attr1(10);
    attrs::constant< double > attr2(5.5);
    attrs::constant< std::string > attr3("Hello, world!");

    attr_set set1;
    set1[data::attr1()] = attr1;
    set1[data::attr2()] = attr2;
    set1[data::attr3()] = attr3;

    attr_set set2 = set1;
    BOOST_REQUIRE_EQUAL(set2.size(), 3UL);

    BOOST_CHECK_EQUAL(set2.erase(data::attr1()), 1UL);
    BOOST_CHECK_EQUAL(set2.size(), 2UL);
    BOOST_CHECK_EQUAL(set2.count(data::attr1()), 0UL);

    BOOST_CHECK_EQUAL(set2.erase(data::attr1()), 0UL);
    BOOST_CHECK_EQUAL(set2.size(), 2UL);

    set2.erase(set2.begin());
    BOOST_CHECK_EQUAL(set2.size(), 1UL);
    BOOST_CHECK_EQUAL(set2.count(data::attr2()), 0UL);

    set2 = set1;
    BOOST_REQUIRE_EQUAL(set2.size(), 3UL);

    attr_set::iterator it = set2.begin();
    set2.erase(++it, set2.end());
    BOOST_CHECK_EQUAL(set2.size(), 1UL);
    BOOST_CHECK_EQUAL(set2.count(data::attr1()), 1UL);
    BOOST_CHECK_EQUAL(set2.count(data::attr2()), 0UL);
    BOOST_CHECK_EQUAL(set2.count(data::attr3()), 0UL);

    set2 = set1;
    BOOST_REQUIRE_EQUAL(set2.size(), 3UL);

    set2.clear();
    BOOST_CHECK(set2.empty());
    BOOST_CHECK_EQUAL(set2.size(), 0UL);
}
