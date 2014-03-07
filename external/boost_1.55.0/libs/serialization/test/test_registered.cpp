/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_registered.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef> // NULL
#include <cstdio> // remove
#include <fstream>

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

class polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & /* ar */, const unsigned int /* file_version */){
    }
public:
    virtual ~polymorphic_base(){};
};

class polymorphic_derived1 : public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
    }
};

class polymorphic_derived2 : public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
    }
};

// save derived polymorphic class
void save_derived(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    polymorphic_derived1 *rd1 = new polymorphic_derived1;
    polymorphic_derived2 *rd2 = new polymorphic_derived2;

    // registration IS necessary when serializing pointers of
    // polymorphic classes
    oa.register_type(static_cast<polymorphic_derived1 *>(NULL));
    oa.register_type(static_cast<polymorphic_derived2 *>(NULL));
    oa << BOOST_SERIALIZATION_NVP(rd1);
    oa << BOOST_SERIALIZATION_NVP(rd2);

    // the above opereration registers the derived classes as a side
    // effect.  Hence, instances can now be correctly serialized through
    // a base class pointer.
    polymorphic_base *rb1 =  rd1;
    polymorphic_base *rb2 =  rd2;
    oa << BOOST_SERIALIZATION_NVP(rb1);
    oa << BOOST_SERIALIZATION_NVP(rb2);

    delete rd1;
    delete rd2;
}

// save derived polymorphic class
void load_derived(const char *testfile)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);

    polymorphic_derived1 *rd1 = NULL;
    polymorphic_derived2 *rd2 = NULL;

    // registration IS necessary when serializing pointers of
    // polymorphic classes
    ia.register_type(static_cast<polymorphic_derived1 *>(NULL));
    ia.register_type(static_cast<polymorphic_derived2 *>(NULL));

    ia >> BOOST_SERIALIZATION_NVP(rd1);

    const boost::serialization::extended_type_info * p1;
    p1 = & boost::serialization::type_info_implementation<polymorphic_derived1>
        ::type::get_const_instance();

    BOOST_CHECK(NULL != p1);

    const boost::serialization::extended_type_info * p2;
    p2 = boost::serialization::type_info_implementation<polymorphic_derived1>
        ::type::get_const_instance().get_derived_extended_type_info(*rd1);

    BOOST_CHECK(NULL != p2);

    BOOST_CHECK_MESSAGE(p1 == p2, "restored pointer d1 not of correct type");

    ia >> BOOST_SERIALIZATION_NVP(rd2);

    BOOST_CHECK_MESSAGE(
        & boost::serialization::type_info_implementation<polymorphic_derived2>
            ::type::get_const_instance()
        == 
        boost::serialization::type_info_implementation<polymorphic_derived2>
            ::type::get_const_instance().get_derived_extended_type_info(*rd2),
        "restored pointer d2 not of correct type"
    );

    polymorphic_base *rb1 = NULL;
    polymorphic_base *rb2 = NULL;

    // the above opereration registers the derived classes as a side
    // effect.  Hence, instances can now be correctly serialized through
    // a base class pointer.
    ia >> BOOST_SERIALIZATION_NVP(rb1);

    BOOST_CHECK_MESSAGE(
        rb1 == dynamic_cast<polymorphic_base *>(rd1),
        "serialized pointers not correctly restored"
    );

    p1 = & boost::serialization::type_info_implementation<polymorphic_derived1>
        ::type::get_const_instance();

    BOOST_CHECK(NULL != p1);

    p2 = boost::serialization::type_info_implementation<polymorphic_base>
        ::type::get_const_instance().get_derived_extended_type_info(*rb1);

    BOOST_CHECK(NULL != p2);

    BOOST_CHECK_MESSAGE(p1 == p2, "restored pointer b1 not of correct type");

    ia >> BOOST_SERIALIZATION_NVP(rb2);

    BOOST_CHECK_MESSAGE(
        rb2 ==  dynamic_cast<polymorphic_base *>(rd2),
        "serialized pointers not correctly restored"
    );

    BOOST_CHECK_MESSAGE(
        & boost::serialization::type_info_implementation<polymorphic_derived2>
            ::type::get_const_instance()
        == boost::serialization::type_info_implementation<polymorphic_base>
            ::type::get_const_instance().get_derived_extended_type_info(*rb2),
        "restored pointer b2 not of correct type"
    );

    delete rb1;
    delete rb2;
}

// save registered polymorphic class
void save_registered(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    polymorphic_base *rb1 = new polymorphic_derived1;
    polymorphic_base *rb2 = new polymorphic_derived2;

    // registration (forward declaration) will permit correct serialization
    // through a pointer to a base class
    oa.register_type(static_cast<polymorphic_derived1 *>(NULL));
    oa.register_type(static_cast<polymorphic_derived2 *>(NULL));
    oa << BOOST_SERIALIZATION_NVP(rb1) << BOOST_SERIALIZATION_NVP(rb2);

    delete rb1;
    delete rb2;
}

// save registered polymorphic class
void load_registered(const char *testfile)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);

    polymorphic_base *rb1 = NULL;
    polymorphic_base *rb2 = NULL;

    // registration (forward declaration) will permit correct serialization
    // through a pointer to a base class
    ia.register_type(static_cast<polymorphic_derived1 *>(NULL));
    ia.register_type(static_cast<polymorphic_derived2 *>(NULL));
    ia >> BOOST_SERIALIZATION_NVP(rb1) >> BOOST_SERIALIZATION_NVP(rb2);

    BOOST_CHECK_MESSAGE(
        & boost::serialization::type_info_implementation<polymorphic_derived1>
            ::type::get_const_instance()
        == boost::serialization::type_info_implementation<polymorphic_base>
            ::type::get_const_instance().get_derived_extended_type_info(*rb1),
        "restored pointer b1 not of correct type"
    );

    BOOST_CHECK_MESSAGE(
        & boost::serialization::type_info_implementation<polymorphic_derived2>
            ::type::get_const_instance()
        == boost::serialization::type_info_implementation<polymorphic_base>
            ::type::get_const_instance().get_derived_extended_type_info(*rb2),
        "restored pointer b2 not of correct type"
    );

    delete rb1;
    delete rb2;
}

int
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    
    BOOST_REQUIRE(NULL != testfile);

    save_derived(testfile);
    load_derived(testfile);
    save_registered(testfile);
    load_registered(testfile);

    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
