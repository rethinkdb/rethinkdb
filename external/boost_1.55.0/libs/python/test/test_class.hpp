// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef TEST_CLASS_DWA2002326_HPP
# define TEST_CLASS_DWA2002326_HPP
# include <boost/detail/lightweight_test.hpp>

template <int n = 0>
struct test_class
{
    explicit test_class(int x) : x(x), magic(7654321 + n) { ++counter; }
    test_class(test_class const& rhs) : x(rhs.x), magic(7654321 + n) { ++counter; }
    virtual ~test_class() { BOOST_TEST(magic == 7654321 + n); magic = 6666666; x = 9999; --counter; }

    void set(int _x) { BOOST_TEST(magic == 7654321 + n); this->x = _x; }
    int value() const { BOOST_TEST(magic == 7654321 + n); return x; }
    operator int() const { return x; }
    static int count() { return counter; }

    int x;
    long magic;
    static int counter;
    
 private:
    void operator=(test_class const&);
};

template <int n>
int test_class<n>::counter;

#endif // TEST_CLASS_DWA2002326_HPP
