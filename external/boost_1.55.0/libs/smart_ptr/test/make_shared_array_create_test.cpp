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
    static int instances;
    explicit type(int a=0, int b=0, int c=0, int d=0, int e=0, int f=0, int g=0, int h=0, int i=0)
        : a(a), b(b), c(c), d(d), e(e), f(f), g(g), h(h), i(i) {
        instances++;
    }
    ~type() {
        instances--;
    }
    const int a;
    const int b;
    const int c;
    const int d;
    const int e;
    const int f;
    const int g;
    const int h;
    const int i;
private:
    type(const type&);
    type& operator=(const type&);
};

int type::instances = 0;

int main() {
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[]> a1 = boost::make_shared<type[]>(2, 1, 2, 3, 4, 5, 6, 7, 8, 9);
        BOOST_TEST(type::instances == 2);
        BOOST_TEST(a1[0].a == 1);
        BOOST_TEST(a1[0].d == 4);
        BOOST_TEST(a1[1].f == 6);
        BOOST_TEST(a1[1].i == 9);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[2]> a1 = boost::make_shared<type[2]>(1, 2, 3, 4, 5, 6, 7, 8, 9);
        BOOST_TEST(type::instances == 2);
        BOOST_TEST(a1[0].a == 1);
        BOOST_TEST(a1[0].d == 4);
        BOOST_TEST(a1[1].f == 6);
        BOOST_TEST(a1[1].i == 9);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[][2]> a1 = boost::make_shared<type[][2]>(2, 1, 2, 3, 4, 5, 6, 7);
        BOOST_TEST(type::instances == 4);
        BOOST_TEST(a1[0][0].a == 1);
        BOOST_TEST(a1[0][1].d == 4);
        BOOST_TEST(a1[1][0].f == 6);
        BOOST_TEST(a1[1][1].i == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[2][2]> a1 = boost::make_shared<type[2][2]>(1, 2, 3, 4, 5, 6, 7);
        BOOST_TEST(type::instances == 4);
        BOOST_TEST(a1[0][0].a == 1);
        BOOST_TEST(a1[0][1].d == 4);
        BOOST_TEST(a1[1][0].f == 6);
        BOOST_TEST(a1[1][1].i == 0);
    }    
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[][2][2]> a1 = boost::make_shared<type[][2][2]>(2, 1, 2, 3, 4, 5);
        BOOST_TEST(type::instances == 8);
        BOOST_TEST(a1[0][0][0].a == 1);
        BOOST_TEST(a1[0][1][0].c == 3);
        BOOST_TEST(a1[1][0][1].e == 5);
        BOOST_TEST(a1[1][1][1].i == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[2][2][2]> a1 = boost::make_shared<type[2][2][2]>(1, 2, 3, 4, 5);
        BOOST_TEST(type::instances == 8);
        BOOST_TEST(a1[0][0][0].a == 1);
        BOOST_TEST(a1[0][1][0].c == 3);
        BOOST_TEST(a1[1][0][1].e == 5);
        BOOST_TEST(a1[1][1][1].i == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[][2][2][2]> a1 = boost::make_shared<type[][2][2][2]>(2, 1, 2, 3);
        BOOST_TEST(type::instances == 16);
        BOOST_TEST(a1[0][0][0][1].a == 1);
        BOOST_TEST(a1[0][0][1][0].c == 3);
        BOOST_TEST(a1[0][1][0][0].f == 0);
        BOOST_TEST(a1[1][0][0][0].i == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[2][2][2][2]> a1 = boost::make_shared<type[2][2][2][2]>(1, 2, 3);
        BOOST_TEST(type::instances == 16);
        BOOST_TEST(a1[0][0][0][1].a == 1);
        BOOST_TEST(a1[0][0][1][0].c == 3);
        BOOST_TEST(a1[0][1][0][0].f == 0);
        BOOST_TEST(a1[1][0][0][0].i == 0);
    }
#endif
    return boost::report_errors();
}
