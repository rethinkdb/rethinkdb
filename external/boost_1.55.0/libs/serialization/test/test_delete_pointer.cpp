/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_delete_pointer.cpp

// (C) Copyright 2002 Vahan Margaryan. 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef> // NULL
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/serialization/throw_exception.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>

//A holds a pointer to another A, but doesn't own the pointer.
//objCount
class A
{
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive &ar, const unsigned int /* file_version */) const
    {
        ar << BOOST_SERIALIZATION_NVP(next_);
    }
    template<class Archive>
    void load(Archive & ar, const unsigned int /* file_version */)
    {
        static int i = 0;
        ar >> BOOST_SERIALIZATION_NVP(next_);
        if(++i == 3)
            boost::serialization::throw_exception(boost::archive::archive_exception(
                boost::archive::archive_exception::no_exception
            ));
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
public:
    A()
    {
        next_ = 0;
        ++objcount;
    }
    A(const A& a)
    {
        next_ = a.next_; ++objcount;
    }
    ~A()
    {
        --objcount;
    }
    A* next_;
    static int objcount;
};


int A::objcount = 0;

int
test_main( int /* argc */, char* /* argv */[] )
{
    std::vector<A*> vec;
    A* a = new A;
    a->next_ = 0;
    vec.push_back(a);

    //fill the vector with chained A's. The vector is assumed
    //to own the objects - we will destroy the objects through this vector.
    unsigned int i;
    for(i   = 1; i < 10; ++i)
    {
        a = new A;
        vec[i - 1]->next_ = a;
        a->next_ = 0;
        vec.push_back(a);
    }

    const char * testfile = boost::archive::tmpnam(0);
    BOOST_REQUIRE(NULL != testfile);

    //output the vector
    {
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << BOOST_SERIALIZATION_NVP(vec);
    }

    //erase the objects
    for(i = 0; i < vec.size(); ++i)
        delete vec[i];
    vec.clear();

    //read the vector back
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        BOOST_TRY {
            ia >> BOOST_SERIALIZATION_NVP(vec);
        }
        BOOST_CATCH (...){
            ia.delete_created_pointers();
            vec.clear();
        }
        BOOST_CATCH_END
    }

    //delete the objects
    for(i = 0; i < vec.size(); ++i)
        delete vec[i];
    vec.clear();

    //identify the leaks
    BOOST_CHECK(A::objcount == 0);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

