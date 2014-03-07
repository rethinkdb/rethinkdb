/*=============================================================================
    Copyright (c) 2010-2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Some very light testing for quickbook::value and friends.
// Just for a few issues that came up during development.

#include <boost/detail/lightweight_test.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <vector>
#include "values.hpp"
#include "files.hpp"

void empty_tests()
{
    quickbook::value q;
    BOOST_TEST(q.empty());
    BOOST_TEST(!q.is_list());
    BOOST_TEST(!q.is_encoded());
}

void qbk_tests()
{
    std::string source = "Source";
    quickbook::value q;
    {
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        q = quickbook::qbk_value(
            fake_file,
            fake_file->source().begin(),
            fake_file->source().end());
    }
    BOOST_TEST_EQ(q.get_quickbook(), boost::string_ref(source));
}

void sort_test()
{
    quickbook::value_builder b;
    b.insert(quickbook::encoded_value("a", 10));
    b.insert(quickbook::encoded_value("b", 2));
    b.insert(quickbook::encoded_value("c", 5));
    b.insert(quickbook::encoded_value("d", 8));
    b.sort_list();
    
    quickbook::value_consumer c = b.release();
    BOOST_TEST(c.check(2)); BOOST_TEST_EQ(c.consume(2).get_encoded(), "b");
    BOOST_TEST(c.check(5)); c.consume(5);
    BOOST_TEST(c.check(8)); c.consume(8);
    BOOST_TEST(c.check(10)); c.consume(10);
    BOOST_TEST(!c.check());
}

void multiple_list_test()
{
    quickbook::value_builder list1;
    quickbook::value_builder list2;

    list1.insert(quickbook::encoded_value("b", 10));

    {
        quickbook::value p1 = quickbook::encoded_value("a", 5);
        list1.insert(p1);
        list2.insert(p1);
    }

    list2.insert(quickbook::encoded_value("c", 3));

    quickbook::value_consumer l1 = list1.release();
    quickbook::value_consumer l2 = list2.release();

    BOOST_TEST(l1.check(10));
    BOOST_TEST_EQ(l1.consume(10).get_encoded(), "b");
    BOOST_TEST(l1.check(5));
    BOOST_TEST_EQ(l1.consume(5).get_encoded(), "a");
    BOOST_TEST(!l1.check());

    BOOST_TEST(l2.check(5));
    BOOST_TEST_EQ(l2.consume(5).get_encoded(), "a");
    BOOST_TEST(l2.check(3));
    BOOST_TEST_EQ(l2.consume(3).get_encoded(), "c");
    BOOST_TEST(!l2.check());
}

void equality_tests()
{
    std::vector<quickbook::value> distinct_values;

    quickbook::value_builder builder;
    quickbook::value nil;
    
    // 0: nil
    distinct_values.push_back(nil);

    // 1: []
    distinct_values.push_back(builder.release());

    // 2: [nil]
    builder.insert(nil);
    distinct_values.push_back(builder.release());

    for(int i = 0; i < distinct_values.size(); ++i)
        for(int j = 0; j < distinct_values.size(); ++j)
            if ((i == j) != (distinct_values[i] == distinct_values[j]))
            {
                BOOST_ERROR("Value mismatch.");
                BOOST_LIGHTWEIGHT_TEST_OSTREAM
                    << "\tat " << i << ", " << j << std::endl;
            }
}

int main()
{
    empty_tests();
    qbk_tests();
    sort_test();
    multiple_list_test();
    equality_tests();

    return boost::report_errors();
}
