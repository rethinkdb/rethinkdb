/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2010 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/functional/hash.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/support_utree.hpp>

#include <iostream>
#include <sstream>
#include <cstdlib>

inline bool check(boost::spirit::utree const& val, std::string expected)
{
    std::stringstream s;
    s << val;
    if (s.str() == expected + " ")
        return true;

    std::cerr << "got result: " << s.str() 
              << ", expected: " << expected << std::endl;
    return false;
}

struct one_two_three
{
    boost::spirit::utree operator()(boost::spirit::utree) const
    {
        return boost::spirit::utree(123);
    }
};

struct this_
{
    boost::spirit::utree operator()(boost::spirit::utree) const
    {
        return boost::spirit::utree(static_cast<int>(boost::hash_value(this)));
    }
};

int main()
{
    using boost::spirit::utree;
    using boost::spirit::get;
    using boost::spirit::utf8_symbol_type;
    using boost::spirit::binary_string_type;

    {
        // test the size
        std::cout << "size of utree is: "
            << sizeof(utree) << " bytes" << std::endl;
        BOOST_TEST_EQ(sizeof(utree), sizeof(void*[4]));
    }

    {
        using boost::spirit::nil;

        utree val(nil);
        BOOST_TEST(check(val, "<nil>"));
    }

    {
        using boost::spirit::empty_list;

        utree val(empty_list);
        BOOST_TEST(check(val, "( )"));
    }

    {
        utree val(true);
        BOOST_TEST(check(val, "true"));
    }

    {
        utree val(123);
        BOOST_TEST(check(val, "123"));
    }

    {
        // single element string
        utree val('x');
        BOOST_TEST(check(val, "\"x\""));
        
        // empty string 
        utree val1("");
        BOOST_TEST(check(val1, "\"\""));
    }

    {
        utree val(123.456);
        BOOST_TEST(check(val, "123.456"));
    }

    { // strings
        utree val("Hello, World");
        BOOST_TEST(check(val, "\"Hello, World\""));
        utree val2;
        val2 = val;
        BOOST_TEST(check(val2, "\"Hello, World\""));
        utree val3("Hello, World. Chuckie is back!!!");
        val = val3;
        BOOST_TEST(check(val, "\"Hello, World. Chuckie is back!!!\""));

        utree val4("Apple");
        utree val5("Apple");
        BOOST_TEST_EQ(val4, val5);

        utree val6("ApplePie");
        BOOST_TEST(val4 < val6);
    }

    { // symbols
        utree val(utf8_symbol_type("Hello, World"));
        BOOST_TEST(check(val, "Hello, World"));
        utree val2;
        val2 = val;
        BOOST_TEST(check(val2, "Hello, World"));
        utree val3(utf8_symbol_type("Hello, World. Chuckie is back!!!"));
        val = val3;
        BOOST_TEST(check(val, "Hello, World. Chuckie is back!!!"));

        utree val4(utf8_symbol_type("Apple"));
        utree val5(utf8_symbol_type("Apple"));
        BOOST_TEST_EQ(val4, val5);

        utree val6(utf8_symbol_type("ApplePie"));
        BOOST_TEST(val4 < val6);
    }

    { // binary_strings
        utree val(binary_string_type("\xDE#\xAD"));
        BOOST_TEST(check(val, "#de23ad#" /* FIXME?: "#\xDE#\xAD#" */));
        utree val2;
        val2 = val;
        BOOST_TEST(check(val2, "#de23ad#" /* FIXME?: "#\xDE#\xAD#" */));
        utree val3(binary_string_type("\xDE\xAD\xBE\xEF"));
        val = val3;
        BOOST_TEST(check(val, "#deadbeef#" /* FIXME?: "#\xDE\xAD\xBE\xEF#" */));

        utree val4(binary_string_type("\x01"));
        utree val5(binary_string_type("\x01"));
        BOOST_TEST_EQ(val4, val5);

        utree val6(binary_string_type("\x01\x02"));
        BOOST_TEST(val4 < val6);
    }

    {
        using boost::spirit::nil;

        utree val;
        val.push_back(123);
        val.push_back("Chuckie");
        BOOST_TEST_EQ(val.size(), 2U);
        utree val2;
        val2.push_back(123.456);
        val2.push_back("Mah Doggie");
        val.push_back(val2);
        BOOST_TEST_EQ(val.size(), 3U);
        BOOST_TEST(check(val, "( 123 \"Chuckie\" ( 123.456 \"Mah Doggie\" ) )"));
        BOOST_TEST(check(val.front(), "123"));

        utree val3(nil);
        val3.swap(val);
        BOOST_TEST_EQ(val3.size(), 3U);
        BOOST_TEST(check(val, "<nil>"));
        val3.swap(val);
        BOOST_TEST(check(val, "( 123 \"Chuckie\" ( 123.456 \"Mah Doggie\" ) )"));
        val.push_back("another string");
        BOOST_TEST_EQ(val.size(), 4U);
        BOOST_TEST(check(val, "( 123 \"Chuckie\" ( 123.456 \"Mah Doggie\" ) \"another string\" )"));
        val.pop_front();
        BOOST_TEST(check(val, "( \"Chuckie\" ( 123.456 \"Mah Doggie\" ) \"another string\" )"));
        utree::iterator i = val.begin();
        ++++i;
        val.insert(i, "Right in the middle");
        BOOST_TEST_EQ(val.size(), 4U);
        BOOST_TEST(check(val, "( \"Chuckie\" ( 123.456 \"Mah Doggie\" ) \"Right in the middle\" \"another string\" )"));
        val.pop_back();
        BOOST_TEST(check(val, "( \"Chuckie\" ( 123.456 \"Mah Doggie\" ) \"Right in the middle\" )"));
        BOOST_TEST_EQ(val.size(), 3U);
        utree::iterator it = val.end(); --it;
        val.erase(it);
        BOOST_TEST(check(val, "( \"Chuckie\" ( 123.456 \"Mah Doggie\" ) )"));
        BOOST_TEST_EQ(val.size(), 2U);

        val.insert(val.begin(), val2.begin(), val2.end());
        BOOST_TEST(check(val, "( 123.456 \"Mah Doggie\" \"Chuckie\" ( 123.456 \"Mah Doggie\" ) )"));
        BOOST_TEST_EQ(val.size(), 4U);
    }

    {
        utree val;
        val.insert(val.end(), 123);
        val.insert(val.end(), "Mia");
        val.insert(val.end(), "Chuckie");
        val.insert(val.end(), "Poly");
        val.insert(val.end(), "Mochi");
        BOOST_TEST(check(val, "( 123 \"Mia\" \"Chuckie\" \"Poly\" \"Mochi\" )"));
    }

    {
        using boost::spirit::nil;
        using boost::spirit::invalid;

        utree a(nil), b(nil);
        BOOST_TEST_EQ(a, b);
        a = 123;
        BOOST_TEST(a != b);
        b = 123;
        BOOST_TEST_EQ(a, b);
        a = 100.00;
        BOOST_TEST(a < b);

        b = a = utree(invalid);
        BOOST_TEST_EQ(a, b);
        a.push_back(1);
        a.push_back("two");
        a.push_back(3.0);
        b.push_back(1);
        b.push_back("two");
        b.push_back(3.0);
        BOOST_TEST_EQ(a, b);
        b.push_back(4);
        BOOST_TEST(a != b);
        BOOST_TEST(a < b);
    }

    {
        using boost::spirit::empty_list;

        utree a(empty_list);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);
        a.push_back(0);

        for (utree::size_type i = 0; i < a.size(); ++i)
            get(a, i) = int(i + 1);

        BOOST_TEST_EQ(get(a, 0), utree(1));
        BOOST_TEST_EQ(get(a, 1), utree(2));
        BOOST_TEST_EQ(get(a, 2), utree(3));
        BOOST_TEST_EQ(get(a, 3), utree(4));
        BOOST_TEST_EQ(get(a, 4), utree(5));
        BOOST_TEST_EQ(get(a, 5), utree(6));
        BOOST_TEST_EQ(get(a, 6), utree(7));
        BOOST_TEST_EQ(get(a, 7), utree(8));
        BOOST_TEST_EQ(get(a, 8), utree(9));
        BOOST_TEST_EQ(get(a, 9), utree(10));
        BOOST_TEST_EQ(get(a, 10), utree(11));
        BOOST_TEST_EQ(get(a, 11), utree(12));
    }

    {
        // test empty list
        utree a;
        a.push_back(1);
        a.pop_front();
        BOOST_TEST(check(a, "( )"));

        // the other way around
        utree b;
        b.push_front(1);
        b.pop_back();
        BOOST_TEST(check(b, "( )"));
    }

    { // test references
        utree val(123);
        utree ref(boost::ref(val));
        BOOST_TEST(check(ref, "123"));
        BOOST_TEST_EQ(ref, utree(123));

        val.clear();
        val.push_back(1);
        val.push_back(2);
        val.push_back(3);
        val.push_back(4);
        BOOST_TEST(check(ref, "( 1 2 3 4 )"));
        BOOST_TEST_EQ(get(ref, 0), utree(1));
        BOOST_TEST_EQ(get(ref, 1), utree(2));
        BOOST_TEST_EQ(get(ref, 2), utree(3));
        BOOST_TEST_EQ(get(ref, 3), utree(4));
    }

    { // put it in an array

        utree vals[] = {
            utree(123),
            utree("Hello, World"),
            utree(123.456)
        };

        BOOST_TEST(check(vals[0], "123"));
        BOOST_TEST(check(vals[1], "\"Hello, World\""));
        BOOST_TEST(check(vals[2], "123.456"));
    }

    { // operators

        BOOST_TEST((utree(false) && utree(false)) == utree(false));
        BOOST_TEST((utree(false) && utree(true))  == utree(false));
        BOOST_TEST((utree(true)  && utree(false)) == utree(false));
        BOOST_TEST((utree(true)  && utree(true))  == utree(true));
        
        BOOST_TEST((utree(0) && utree(0)) == utree(false));
        BOOST_TEST((utree(0) && utree(1)) == utree(false));
        BOOST_TEST((utree(1) && utree(0)) == utree(false));
        BOOST_TEST((utree(1) && utree(1)) == utree(true));
        
        BOOST_TEST((utree(false) || utree(false)) == utree(false));
        BOOST_TEST((utree(false) || utree(true))  == utree(true));
        BOOST_TEST((utree(true)  || utree(false)) == utree(true));
        BOOST_TEST((utree(true)  || utree(true))  == utree(true));
        
        BOOST_TEST((utree(0) || utree(0)) == utree(false));
        BOOST_TEST((utree(0) || utree(1)) == utree(true));
        BOOST_TEST((utree(1) || utree(0)) == utree(true));
        BOOST_TEST((utree(1) || utree(1)) == utree(true));

        BOOST_TEST(!utree(true)   == utree(false));
        BOOST_TEST(!utree(false)  == utree(true));
        BOOST_TEST(!utree(1)      == utree(false));
        BOOST_TEST(!utree(0)      == utree(true));

        BOOST_TEST((utree(456) + utree(123)) == utree(456 + 123));
        BOOST_TEST((utree(456) + utree(123.456)) == utree(456 + 123.456));
        BOOST_TEST((utree(456) - utree(123)) == utree(456 - 123));
        BOOST_TEST((utree(456) - utree(123.456)) == utree(456 - 123.456));
        BOOST_TEST((utree(456) * utree(123)) == utree(456 * 123));
        BOOST_TEST((utree(456) * utree(123.456)) == utree(456 * 123.456));
        BOOST_TEST((utree(456) / utree(123)) == utree(456 / 123));
        BOOST_TEST((utree(456) / utree(123.456)) == utree(456 / 123.456));
        BOOST_TEST((utree(456) % utree(123)) == utree(456 % 123));
        BOOST_TEST(-utree(456) == utree(-456));

        BOOST_TEST((utree(456) & utree(123)) == utree(456 & 123));
        BOOST_TEST((utree(456) | utree(123)) == utree(456 | 123));
        BOOST_TEST((utree(456) ^ utree(123)) == utree(456 ^ 123));
        BOOST_TEST((utree(456) << utree(3)) == utree(456 << 3));
        BOOST_TEST((utree(456) >> utree(2)) == utree(456 >> 2));
        BOOST_TEST(~utree(456) == utree(~456));
    }

    { // test reference iterator
        utree val;
        val.push_back(1);
        val.push_back(2);
        val.push_back(3);
        val.push_back(4);
        BOOST_TEST(check(val, "( 1 2 3 4 )"));

        utree::ref_iterator b = val.ref_begin();
        utree::ref_iterator e = val.ref_end();

        utree ref(boost::make_iterator_range(b, e));
        BOOST_TEST_EQ(get(ref, 0), utree(1));
        BOOST_TEST_EQ(get(ref, 1), utree(2));
        BOOST_TEST_EQ(get(ref, 2), utree(3));
        BOOST_TEST_EQ(get(ref, 3), utree(4));
        BOOST_TEST(check(ref, "( 1 2 3 4 )"));
    }

    {
        // check the tag
        // TODO: test tags on all utree types 
        utree x;
        x.tag(123);
        BOOST_TEST_EQ(x.tag(), 123);

        x = "hello world! my name is bob the builder";
        x.tag(123);
        BOOST_TEST_EQ(x.tag(), 123);

        x.tag(456);
        BOOST_TEST_EQ(x.tag(), 456);
        BOOST_TEST_EQ(x.size(), 39U);
        BOOST_TEST(check(x, "\"hello world! my name is bob the builder\""));

        x = "hello";
        x.tag(456);
        BOOST_TEST_EQ(x.tag(), 456);

        x.tag(789);
        BOOST_TEST_EQ(x.tag(), 789);
        BOOST_TEST_EQ(x.size(), 5U);
        BOOST_TEST(check(x, "\"hello\""));
    }

    {
        // test functions
        using boost::spirit::stored_function;

        utree f = stored_function<one_two_three>();
        f.eval(utree());
    }
    
    {
        // test referenced functions
        using boost::spirit::referenced_function;

        one_two_three f;
        utree ff = referenced_function<one_two_three>(f);
        BOOST_TEST_EQ(ff.eval(utree()), f(utree()));
    }

    {
        // shallow ranges
        using boost::spirit::shallow;

        utree val;
        val.push_back(1);
        val.push_back(2);
        val.push_back(3);
        val.push_back(4);

        utree::iterator i = val.begin(); ++i;
        utree alias(utree::range(i, val.end()), shallow);

        BOOST_TEST(check(alias, "( 2 3 4 )"));
        BOOST_TEST_EQ(alias.size(), 3U);
        BOOST_TEST_EQ(alias.front(), 2);
        BOOST_TEST_EQ(alias.back(), 4);
        BOOST_TEST(!alias.empty());
        BOOST_TEST_EQ(get(alias, 1), 3);
    }

    {
        // shallow string ranges
        using boost::spirit::utf8_string_range_type;
        using boost::spirit::shallow;

        char const* s = "Hello, World";
        utree val(utf8_string_range_type(s, s + strlen(s)), shallow);
        BOOST_TEST(check(val, "\"Hello, World\""));

        utf8_string_range_type r = val.get<utf8_string_range_type>();
        utf8_string_range_type pf(r.begin()+1, r.end()-1);
        val = utree(pf, shallow);
        BOOST_TEST(check(val, "\"ello, Worl\""));
    }

    {
        // any pointer
        using boost::spirit::any_ptr;

        int n = 123;
        utree up = any_ptr(&n);
        BOOST_TEST(*up.get<int*>() == 123);
    }

    // tags
    {
        short min = (std::numeric_limits<short>::min)();
        short max = (std::numeric_limits<short>::max)();

        utree::list_type u;
        utree u2;
        bool ok = true;

        for (int t = min ; ok && t <= max ; ++t) {
            u.tag(t);
            u2 = u;
            BOOST_TEST_EQ(t, u.tag());
            BOOST_TEST_EQ(t, u2.tag());
            ok = t == u.tag() && t == u2.tag();
            u2 = utree("12");
        }
    }

    return boost::report_errors();
}
