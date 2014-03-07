/*
 * Copyright (c) 2012 Glen Joseph Fernandes 
 * glenfe at live dot com
 *
 * Distributed under the Boost Software License, 
 * Version 1.0. (See accompanying file LICENSE_1_0.txt 
 * or copy at http://boost.org/LICENSE_1_0.txt)
 */
#include <boost/detail/lightweight_test.hpp>
#include <boost/smart_ptr/allocate_shared_array.hpp>

class type {
public:
    type(int value) 
        : value(value) {
    }
    const int value;
private:
    type& operator=(const type&);
};

int main() {
#if !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
    {
        boost::shared_ptr<int[4]> a1 = boost::allocate_shared<int[4]>(std::allocator<int>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0] == 0);
        BOOST_TEST(a1[1] == 1);
        BOOST_TEST(a1[2] == 2);
        BOOST_TEST(a1[3] == 3);
    }
    {
        boost::shared_ptr<const int[4]> a1 = boost::allocate_shared<const int[4]>(std::allocator<int>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0] == 0);
        BOOST_TEST(a1[1] == 1);
        BOOST_TEST(a1[2] == 2);
        BOOST_TEST(a1[3] == 3);
    }
    {
        boost::shared_ptr<type[4]> a1 = boost::allocate_shared<type[4]>(std::allocator<type>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0].value == 0);
        BOOST_TEST(a1[1].value == 1);
        BOOST_TEST(a1[2].value == 2);
        BOOST_TEST(a1[3].value == 3);
    }
    {
        boost::shared_ptr<const type[4]> a1 = boost::allocate_shared<const type[4]>(std::allocator<type>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0].value == 0);
        BOOST_TEST(a1[1].value == 1);
        BOOST_TEST(a1[2].value == 2);
        BOOST_TEST(a1[3].value == 3);
    }
#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    {
        boost::shared_ptr<int[]> a1 = boost::allocate_shared<int[]>(std::allocator<int>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0] == 0);
        BOOST_TEST(a1[1] == 1);
        BOOST_TEST(a1[2] == 2);
        BOOST_TEST(a1[3] == 3);
    }
    {
        boost::shared_ptr<const int[]> a1 = boost::allocate_shared<const int[]>(std::allocator<int>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0] == 0);
        BOOST_TEST(a1[1] == 1);
        BOOST_TEST(a1[2] == 2);
        BOOST_TEST(a1[3] == 3);
    }
    {
        boost::shared_ptr<type[]> a1 = boost::allocate_shared<type[]>(std::allocator<type>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0].value == 0);
        BOOST_TEST(a1[1].value == 1);
        BOOST_TEST(a1[2].value == 2);
        BOOST_TEST(a1[3].value == 3);
    }
    {
        boost::shared_ptr<const type[]> a1 = boost::allocate_shared<const type[]>(std::allocator<type>(), { 0, 1, 2, 3 });
        BOOST_TEST(a1[0].value == 0);
        BOOST_TEST(a1[1].value == 1);
        BOOST_TEST(a1[2].value == 2);
        BOOST_TEST(a1[3].value == 3);
    }
#endif
#endif
    return boost::report_errors();
}
