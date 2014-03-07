/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_derived.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef> // NULL
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include <boost/archive/archive_exception.hpp>
#include "test_tools.hpp"

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/type_info_implementation.hpp>

class base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & /* ar */, const unsigned int /* file_version */){
    }
protected:
    virtual ~base(){};
};

class derived1 : public base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(base);
    }
};

class derived2 : public base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(base);
    }
};

// save non-polymorphic classes through a base class pointer
void save_derived(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    // registration not necessary when serializing the most derived pointer
    derived1 *d1 = new derived1;
    derived2 *d2 = new derived2;
    oa << BOOST_SERIALIZATION_NVP(d1) << BOOST_SERIALIZATION_NVP(d2);

    // upcasting non-polymorphic pointers may not lead to the expected
    // result. In the current type id system
    base *b1 = d1;
    base *b2 = d2;
    
    // Warning, the current type id system does not yield true
    // type id for non-polymorphic types
    const boost::serialization::extended_type_info & this_type
        = boost::serialization::type_info_implementation<base>::type
            ::get_const_instance();
    // retrieve the true type of the object pointed to
    const boost::serialization::extended_type_info & true_type
        = * boost::serialization::type_info_implementation<base>::type
            ::get_const_instance().get_derived_extended_type_info(*b1);

    BOOST_WARN_MESSAGE(
        !(this_type == true_type), 
        "current type id system does not support non-polymorphic types"
    );

    oa << BOOST_SERIALIZATION_NVP(b1);
    oa << BOOST_SERIALIZATION_NVP(b2);

    delete d1;
    delete d2;
}

// save non-polymorphic classes through a base class pointer
void load_derived(const char *testfile)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);

    // registration not necessary when serializing the most derived pointer
    derived1 *d1 = NULL;
    derived2 *d2 = NULL;
    ia >> BOOST_SERIALIZATION_NVP(d1) >> BOOST_SERIALIZATION_NVP(d2);

    // upcasting non-polymorphic pointers may not lead to the expected
    // result. In the current type id system
    base *b1 = NULL;
    base *b2 = NULL;
    
    // note: this will produce incorrect results for non-polymorphic classes
    ia >> BOOST_SERIALIZATION_NVP(b1);
    ia >> BOOST_SERIALIZATION_NVP(b2);

    // Warning, the current type id system does not yield true
    // type id for non-polymorphic types
    const boost::serialization::extended_type_info & this_type
        = boost::serialization::type_info_implementation<base>::type
            ::get_const_instance();
    // retrieve the true type of the object pointed to
    const boost::serialization::extended_type_info & true_type
        = * boost::serialization::type_info_implementation<base>::type
            ::get_const_instance().get_derived_extended_type_info(*b1);
            
    BOOST_WARN_MESSAGE(
        ! (this_type == true_type), 
        "current type id system does fails for non-polymorphic types"
    );

    BOOST_CHECK(b1 == static_cast<base *>(d1));
    BOOST_CHECK(b2 == static_cast<base *>(d2));

    delete d1;
    delete d2;
}

int
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    save_derived(testfile);
    load_derived(testfile);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
