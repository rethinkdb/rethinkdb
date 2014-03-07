// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/type_id.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <boost/python/converter/pointer_type_id.hpp>

int main()
{
    using namespace boost::python::converter;
    
    boost::python::type_info x
        = boost::python::type_id<int>();
    

    BOOST_TEST(pointer_type_id<int*>() == x);
    BOOST_TEST(pointer_type_id<int const*>() == x);
    BOOST_TEST(pointer_type_id<int volatile*>() == x);
    BOOST_TEST(pointer_type_id<int const volatile*>() == x);
    
    BOOST_TEST(pointer_type_id<int*&>() == x);
    BOOST_TEST(pointer_type_id<int const*&>() == x);
    BOOST_TEST(pointer_type_id<int volatile*&>() == x);
    BOOST_TEST(pointer_type_id<int const volatile*&>() == x);
    
    BOOST_TEST(pointer_type_id<int*const&>() == x);
    BOOST_TEST(pointer_type_id<int const*const&>() == x);
    BOOST_TEST(pointer_type_id<int volatile*const&>() == x);
    BOOST_TEST(pointer_type_id<int const volatile*const&>() == x);
    
    BOOST_TEST(pointer_type_id<int*volatile&>() == x);
    BOOST_TEST(pointer_type_id<int const*volatile&>() == x);
    BOOST_TEST(pointer_type_id<int volatile*volatile&>() == x);
    BOOST_TEST(pointer_type_id<int const volatile*volatile&>() == x);
    
    BOOST_TEST(pointer_type_id<int*const volatile&>() == x);
    BOOST_TEST(pointer_type_id<int const*const volatile&>() == x);
    BOOST_TEST(pointer_type_id<int volatile*const volatile&>() == x);
    BOOST_TEST(pointer_type_id<int const volatile*const volatile&>() == x);
    
    return boost::report_errors();
}
