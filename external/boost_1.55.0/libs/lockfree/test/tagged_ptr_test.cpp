//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <limits>

#include <boost/lockfree/detail/tagged_ptr.hpp>

#define BOOST_TEST_MAIN
#ifdef BOOST_LOCKFREE_INCLUDE_TESTS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

BOOST_AUTO_TEST_CASE( tagged_ptr_test )
{
    using namespace boost::lockfree::detail;
    int a(1), b(2);

    typedef tagged_ptr<int>::tag_t tag_t;
    const tag_t max_tag = (std::numeric_limits<tag_t>::max)();

    {
        tagged_ptr<int> i (&a, 0);
        tagged_ptr<int> j (&b, 1);

        i = j;

        BOOST_REQUIRE_EQUAL(i.get_ptr(), &b);
        BOOST_REQUIRE_EQUAL(i.get_tag(), 1);
    }

    {
        tagged_ptr<int> i (&a, 0);
        tagged_ptr<int> j (i);

        BOOST_REQUIRE_EQUAL(i.get_ptr(), j.get_ptr());
        BOOST_REQUIRE_EQUAL(i.get_tag(), j.get_tag());
    }

    {
        tagged_ptr<int> i (&a, 0);
        BOOST_REQUIRE_EQUAL(i.get_tag() + 1, i.get_next_tag());
    }

    {
        tagged_ptr<int> j (&a, max_tag);
        BOOST_REQUIRE_EQUAL(j.get_next_tag(), 0);
    }

    {
        tagged_ptr<int> j (&a, max_tag - 1);
        BOOST_REQUIRE_EQUAL(j.get_next_tag(), max_tag);
    }
}
