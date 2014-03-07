/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_no_rtti.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// note: this program tests the inter-operability of different
// extended typeinfo systems.  In this example, one class is
// identified using the default RTTI while the other uses a custom
// system based on the export key.
// 
// As this program uses RTTI for one of the types, the test will fail
// on a system for which RTTI is not enabled or not existent.

#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/archive/archive_exception.hpp>
#include "../test/test_tools.hpp"
#include <boost/preprocessor/stringize.hpp>
// #include <boost/preprocessor/cat.hpp>
// the following fails with (only!) gcc 3.4 
// #include BOOST_PP_STRINGIZE(BOOST_PP_CAT(../test/,BOOST_ARCHIVE_TEST))
// just copy over the files from the test directory
#include BOOST_PP_STRINGIZE(BOOST_ARCHIVE_TEST)

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/type_info_implementation.hpp>
#include <boost/serialization/extended_type_info_no_rtti.hpp>

class polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & /* ar */, const unsigned int /* file_version */){
    }
public:
    virtual const char * get_key() const = 0;
    virtual ~polymorphic_base(){};
};

BOOST_IS_ABSTRACT(polymorphic_base)
BOOST_CLASS_TYPE_INFO(
    polymorphic_base,
    extended_type_info_no_rtti<polymorphic_base>
)
// note: types which use ...no_rtti MUST be exported
BOOST_CLASS_EXPORT(polymorphic_base)

class polymorphic_derived1 : public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int  /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
    }
public:
    virtual const char * get_key() const ;
};

BOOST_CLASS_TYPE_INFO(
    polymorphic_derived1,
    extended_type_info_no_rtti<polymorphic_derived1>
)
BOOST_CLASS_EXPORT(polymorphic_derived1)

const char * polymorphic_derived1::get_key() const {
    const boost::serialization::extended_type_info *eti
        = boost::serialization::type_info_implementation<polymorphic_derived1>
            ::type::get_instance();
    return eti->get_key();
}

class polymorphic_derived2 : public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
    }
public:
    virtual const char * get_key() const ;
};

// note the mixing of type_info systems is supported.
BOOST_CLASS_TYPE_INFO(
    polymorphic_derived2,
    boost::serialization::extended_type_info_typeid<polymorphic_derived2>
)

BOOST_CLASS_EXPORT(polymorphic_derived2)

const char * polymorphic_derived2::get_key() const {
    // use the exported key as the identifier
    const boost::serialization::extended_type_info *eti
        = boost::serialization::type_info_implementation<polymorphic_derived2>
        ::type::get_instance();
    return eti->get_key();
}

// save derived polymorphic class
void save_derived(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    polymorphic_derived1 *rd1 = new polymorphic_derived1;
    polymorphic_derived2 *rd2 = new polymorphic_derived2;

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

    ia >> BOOST_SERIALIZATION_NVP(rd1);

    BOOST_CHECK_MESSAGE(
        boost::serialization::type_info_implementation<polymorphic_derived1>
            ::type::get_instance()
        == boost::serialization::type_info_implementation<polymorphic_derived1>
            ::type::get_derived_extended_type_info(*rd1),
        "restored pointer d1 not of correct type"
    );

    ia >> BOOST_SERIALIZATION_NVP(rd2);

    BOOST_CHECK_MESSAGE(
        boost::serialization::type_info_implementation<polymorphic_derived2>
            ::type::get_instance()
        == boost::serialization::type_info_implementation<polymorphic_derived2>
            ::type::get_derived_extended_type_info(*rd2),
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

    BOOST_CHECK_MESSAGE(
        boost::serialization::type_info_implementation<polymorphic_derived1>
            ::type::get_instance()
        == boost::serialization::type_info_implementation<polymorphic_base>
            ::type::get_derived_extended_type_info(*rb1),
        "restored pointer b1 not of correct type"
    );

    ia >> BOOST_SERIALIZATION_NVP(rb2);

    BOOST_CHECK_MESSAGE(
        rb2 ==  dynamic_cast<polymorphic_base *>(rd2),
        "serialized pointers not correctly restored"
    );

    BOOST_CHECK_MESSAGE(
        boost::serialization::type_info_implementation<polymorphic_derived2>
            ::type::get_instance()
        == boost::serialization::type_info_implementation<polymorphic_base>
            ::type::get_derived_extended_type_info(*rb2),
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

    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
