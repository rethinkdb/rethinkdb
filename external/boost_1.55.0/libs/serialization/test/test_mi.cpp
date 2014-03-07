/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_mi.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// test of serialization of base classes with multiple inheritance
// contributed by Peter Dimov

#include <cstddef>
#include <iostream>
#include <fstream>

#include <boost/config.hpp>
#include <cstdio> // remove
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/base_object.hpp>

class A
{
private:
    friend class boost::serialization::access;
    int x;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_NVP(x);
    }
public:
    explicit A(int x = 0): x(x) {}
    virtual ~A(); // = 0;
    int get_x() const
    {
        return x;
    }
};

inline A::~A()
{
}

class B
{
private:
    int y;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_NVP(y);
    }
public:
    explicit B(int y = 0): y(y) {}
    virtual ~B(){}
    int get_y() const
    {
        return y;
    }
};

class C: public A, public B
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(A);
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(B);
    }
public:
    C(){}
    C(int x, int y): A(x), B(y){}
};

int
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    C * pc = new C(1, 2);
    A * pa = pc;
    B * pb = pc;

    BOOST_CHECK(pa == pc);
    BOOST_CHECK(pb == pc);

    int x = pa->get_x();
    int y = pb->get_y();

    std::cout << "pa->get_x(): " << pa->get_x() << std::endl;
    std::cout << "pb->get_y(): " << pb->get_y() << std::endl;

    {
        test_ostream ofs(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(ofs);
        oa << BOOST_SERIALIZATION_NVP(pc);
        oa << BOOST_SERIALIZATION_NVP(pa);
        oa << BOOST_SERIALIZATION_NVP(pb);
    }

    delete pc;
    pc = NULL;
    pa = NULL;
    pb = NULL;

    {
        test_istream ifs(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(ifs);
        ia >> BOOST_SERIALIZATION_NVP(pc);
        ia >> BOOST_SERIALIZATION_NVP(pa);
        ia >> BOOST_SERIALIZATION_NVP(pb);
    }

    BOOST_CHECK(pa == pc);
    BOOST_CHECK(pb == pc);

    BOOST_CHECK(x == pa->get_x());
    BOOST_CHECK(y == pb->get_y());

    std::cout << "pa->get_x(): " << pa->get_x() << std::endl;
    std::cout << "pb->get_y(): " << pb->get_y() << std::endl;

    delete pc;
    std::remove(testfile);
    return EXIT_SUCCESS;
}
