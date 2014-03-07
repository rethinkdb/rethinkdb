// demo_shared_ptr.cpp : demonstrates adding serialization to a template

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . Polymorphic
// derived pointer example by David Tonge.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org for updates, documentation, and revision history.

#include <iomanip>
#include <iostream>
#include <cstddef> // NULL
#include <fstream>
#include <string>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/tmpdir.hpp>

#include <boost/serialization/shared_ptr.hpp>

///////////////////////////
// test shared_ptr serialization
class A
{
private:
    friend class boost::serialization::access;
    int x;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & x;
    }
public:
    static int count;
    A(){++count;}    // default constructor
    virtual ~A(){--count;}   // default destructor
};

BOOST_SERIALIZATION_SHARED_PTR(A)

/////////////////
// ADDITION BY DT
class B : public A
{
private:
    friend class boost::serialization::access;
    int x;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & boost::serialization::base_object<A>(*this);
    }
public:
    static int count;
    B() : A() {};
    virtual ~B() {};
};

BOOST_SERIALIZATION_SHARED_PTR(B)

/////////////////

int A::count = 0;

void display(boost::shared_ptr<A> &spa, boost::shared_ptr<A> &spa1)
{
    std::cout << "a = 0x" << std::hex << spa.get() << " ";
    if (spa.get()) std::cout << "is a " << typeid(*(spa.get())).name() << "* ";
    std::cout << "use count = " << std::dec << spa.use_count() << std::endl;
    std::cout << "a1 = 0x" << std::hex << spa1.get() << " ";
    if (spa1.get()) std::cout << "is a " << typeid(*(spa1.get())).name() << "* ";
    std::cout << "use count = " << std::dec << spa1.use_count() << std::endl;
    std::cout << "unique element count = " << A::count << std::endl;
}

int main(int /* argc */, char * /*argv*/[])
{
    std::string filename(boost::archive::tmpdir());
    filename += "/testfile";

    // create  a new shared pointer to ta new object of type A
    boost::shared_ptr<A> spa(new A);
    boost::shared_ptr<A> spa1;
    spa1 = spa;
    display(spa, spa1);
    // serialize it
    {
        std::ofstream ofs(filename.c_str());
        boost::archive::text_oarchive oa(ofs);
        oa << spa;
        oa << spa1;
    }
    // reset the shared pointer to NULL
    // thereby destroying the object of type A
    spa.reset();
    spa1.reset();
    display(spa, spa1);
    // restore state to one equivalent to the original
    // creating a new type A object
    {
        // open the archive
        std::ifstream ifs(filename.c_str());
        boost::archive::text_iarchive ia(ifs);

        // restore the schedule from the archive
        ia >> spa;
        ia >> spa1;
    }
    display(spa, spa1);
    spa.reset();
    spa1.reset();

    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "New tests" << std::endl;

    /////////////////
    // ADDITION BY DT
    // create  a new shared pointer to ta new object of type A
    spa = boost::shared_ptr<A>(new B);
    spa1 = spa;
    display(spa, spa1);
    // serialize it
    {
        std::ofstream ofs(filename.c_str());
        boost::archive::text_oarchive oa(ofs);
        oa.register_type(static_cast<B *>(NULL));
        oa << spa;
        oa << spa1;
    }
    // reset the shared pointer to NULL
    // thereby destroying the object of type B
    spa.reset();
    spa1.reset();
    display(spa, spa1);
    // restore state to one equivalent to the original
    // creating a new type B object
    {
        // open the archive
        std::ifstream ifs(filename.c_str());
        boost::archive::text_iarchive ia(ifs);

        // restore the schedule from the archive
        ia.register_type(static_cast<B *>(NULL));
        ia >> spa;
        ia >> spa1;
    }
    display(spa, spa1);
    ///////////////
    std::remove(filename.c_str());

    // obj of type A gets destroyed
    // as smart_ptr goes out of scope
    return 0;
}
