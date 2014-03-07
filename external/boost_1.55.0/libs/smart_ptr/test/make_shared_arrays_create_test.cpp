/*
 * Copyright (c) 2012 Glen Joseph Fernandes 
 * glenfe at live dot com
 *
 * Distributed under the Boost Software License, 
 * Version 1.0. (See accompanying file LICENSE_1_0.txt 
 * or copy at http://boost.org/LICENSE_1_0.txt)
 */
#include <boost/detail/lightweight_test.hpp>
#include <boost/smart_ptr/make_shared_array.hpp>

class type {
public:
    type(int x, int y) 
        : x(x), y(y) {
    }
    const int x;
    const int y;
private:
    type& operator=(const type&);
};

int main() {
#if !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
    {
        boost::shared_ptr<int[4]> a1 = boost::make_shared<int[4]>({0, 1, 2, 3});
        BOOST_TEST(a1[0] == 0);
        BOOST_TEST(a1[1] == 1);
        BOOST_TEST(a1[2] == 2);
        BOOST_TEST(a1[3] == 3);
    }
    {
        boost::shared_ptr<int[2][2]> a1 = boost::make_shared<int[2][2]>({ {0, 1}, {2, 3} });
        BOOST_TEST(a1[0][0] == 0);
        BOOST_TEST(a1[0][1] == 1);
        BOOST_TEST(a1[1][0] == 2);
        BOOST_TEST(a1[1][1] == 3);
    }
    {
        boost::shared_ptr<int[][2]> a1 = boost::make_shared<int[][2]>(2, {0, 1});
        BOOST_TEST(a1[0][0] == 0);
        BOOST_TEST(a1[0][1] == 1);
        BOOST_TEST(a1[1][0] == 0);
        BOOST_TEST(a1[1][1] == 1);
    }
    {
        boost::shared_ptr<int[][2][2]> a1 = boost::make_shared<int[][2][2]>(2, { {0, 1}, {2, 3} });
        BOOST_TEST(a1[0][0][0] == 0);
        BOOST_TEST(a1[0][0][1] == 1);
        BOOST_TEST(a1[1][1][0] == 2);
        BOOST_TEST(a1[1][1][1] == 3);
    }
    {
        boost::shared_ptr<int[2][2]> a1 = boost::make_shared<int[2][2]>({ 0, 1 });
        BOOST_TEST(a1[0][0] == 0);
        BOOST_TEST(a1[0][1] == 1);
        BOOST_TEST(a1[1][0] == 0);
        BOOST_TEST(a1[1][1] == 1);
    }
    {
        boost::shared_ptr<int[2][2][2]> a1 = boost::make_shared<int[2][2][2]>({ {0, 1}, {2, 3} });
        BOOST_TEST(a1[0][0][0] == 0);
        BOOST_TEST(a1[0][0][1] == 1);
        BOOST_TEST(a1[1][1][0] == 2);
        BOOST_TEST(a1[1][1][1] == 3);
    }
#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    {
        boost::shared_ptr<int[]> a1 = boost::make_shared<int[]>({0, 1, 2, 3});
        BOOST_TEST(a1[0] == 0);
        BOOST_TEST(a1[1] == 1);
        BOOST_TEST(a1[2] == 2);
        BOOST_TEST(a1[3] == 3);
    }
#endif
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    {
        boost::shared_ptr<type[]> a1 = boost::make_shared<type[]>(4, {1, 2});
        BOOST_TEST(a1[0].x == 1);
        BOOST_TEST(a1[1].y == 2);
        BOOST_TEST(a1[2].x == 1);
        BOOST_TEST(a1[3].y == 2);
    }
    {
        boost::shared_ptr<type[4]> a1 = boost::make_shared<type[4]>({1, 2});
        BOOST_TEST(a1[0].x == 1);
        BOOST_TEST(a1[1].y == 2);
        BOOST_TEST(a1[2].x == 1);
        BOOST_TEST(a1[3].y == 2);
    }
#endif
#endif
    return boost::report_errors();
}
