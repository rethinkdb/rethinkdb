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
    static unsigned int instances;
    explicit type(int = 0, int = 0)
        : member() {
        instances++;
    }
    ~type() {
        instances--;
    }
private:
    type(const type&);
    type& operator=(const type&);
    double member;
};

unsigned int type::instances = 0;

int main() {
    {
        boost::shared_ptr<int[][2][2]> a1 = boost::make_shared<int[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(a1[0][0][1] == 0);
        BOOST_TEST(a1[0][1][0] == 0);
        BOOST_TEST(a1[1][0][0] == 0);
    }    
    {
        boost::shared_ptr<const int[][2][2]> a1 = boost::make_shared<const int[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(a1[0][0][1] == 0);
        BOOST_TEST(a1[0][1][0] == 0);
        BOOST_TEST(a1[1][0][0] == 0);
    }    
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[][2][2]> a1 = boost::make_shared<type[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);        
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }    
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<const type[][2][2]> a1 = boost::make_shared<const type[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);        
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[][2][2]> a1 = boost::make_shared<type[][2][2]>(2, 1, 5);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[2][2][2]> a1 = boost::make_shared<type[2][2][2]>(1, 5);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<const type[][2][2]> a1 = boost::make_shared<const type[][2][2]>(2, 1, 5);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<const type[2][2][2]> a1 = boost::make_shared<const type[2][2][2]>(1, 5);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
#endif
    {
        boost::shared_ptr<int[][2][2]> a1 = boost::make_shared_noinit<int[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
    }
    {
        boost::shared_ptr<int[2][2][2]> a1 = boost::make_shared_noinit<int[2][2][2]>();
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
    }    
    {
        boost::shared_ptr<const int[][2][2]> a1 = boost::make_shared_noinit<const int[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
    }
    {
        boost::shared_ptr<const int[2][2][2]> a1 = boost::make_shared_noinit<const int[2][2][2]>();
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
    }    
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[][2][2]> a1 = boost::make_shared_noinit<type[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<type[2][2][2]> a1 = boost::make_shared_noinit<type[2][2][2]>();
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<const type[][2][2]> a1 = boost::make_shared_noinit<const type[][2][2]>(2);
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
    BOOST_TEST(type::instances == 0);
    {
        boost::shared_ptr<const type[2][2][2]> a1 = boost::make_shared_noinit<const type[2][2][2]>();
        BOOST_TEST(a1.get() != 0);
        BOOST_TEST(a1.use_count() == 1);
        BOOST_TEST(type::instances == 8);
        a1.reset();
        BOOST_TEST(type::instances == 0);
    }
    return boost::report_errors();
}
