/*=============================================================================
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
// vim:ts=4:sw=4:et

#undef BOOST_SPIRIT_THREADSAFE
#include <boost/spirit/home/classic/core/non_terminal/impl/object_with_id.ipp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>

using BOOST_SPIRIT_CLASSIC_NS::impl::object_with_id;

struct tag1 {};
struct tag2 {};

typedef object_with_id<tag1> class1;
typedef object_with_id<tag2> class2;

int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n";
    std::cout << "\n";
    std::cout << "          object_with_id test (ST)\n";
    std::cout << "\n";
    std::cout << "/////////////////////////////////////////////////////////\n";
    std::cout << "\n";

    class1 *c1o1 = new class1;
    class1 *c1o2 = new class1;
    class1 *c1o3 = new class1;

    // test wether the objects have consecutive numbers
    BOOST_TEST(c1o1->get_object_id()==1);
    BOOST_TEST(c1o2->get_object_id()==2);
    BOOST_TEST(c1o3->get_object_id()==3);

    // test wether number recycling works
    delete c1o3;
    c1o3 = new class1;
    BOOST_TEST(c1o3->get_object_id()==3);

    delete c1o2;
    c1o2 = new class1;
    BOOST_TEST(c1o2->get_object_id()==2);

    delete c1o2;
    delete c1o3;
    c1o2 = new class1;
    c1o3 = new class1;
    BOOST_TEST(c1o3->get_object_id()==3);
    BOOST_TEST(c1o2->get_object_id()==2);

    // test whether objects of different classes are numbered independently
    class2 *c2o1 = new class2;
    class2 *c2o2 = new class2;
    class2 *c2o3 = new class2;
    BOOST_TEST(c2o1->get_object_id()==1);
    BOOST_TEST(c2o2->get_object_id()==2);
    BOOST_TEST(c2o3->get_object_id()==3);

    //
    delete c1o1;
    delete c2o2;
    c2o2 = new class2;
    c1o1 = new class1;
    BOOST_TEST(c1o1->get_object_id()==1);
    BOOST_TEST(c2o2->get_object_id()==2);

    // test wether the copy ctor doesn't copy the id
    delete c1o1;
    c1o1 = new class1(*c1o2);
    BOOST_TEST(c1o1->get_object_id()==1);

    // test wether the assignment operator doesn't assign the id
    *c1o1 = *c1o2;
    BOOST_TEST(c1o1->get_object_id()==1);

    return boost::report_errors();
}
